#include "loader/CsvLoader.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <exception>
#include <limits>
#include <optional>
#include <atomic>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include "CodeM/core/Complex.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/ScalarType.h"
#include "loader/SourceResolver.h"
#include "table/DataFrame.h"
#include "table/Slate.h"
#include "table/io/CsvSchemaConvert.h"
#include "loader/parallel_csv_reader.h"

namespace pluss::loader {

namespace {

struct CsvRecordBuildProfile {
  double filter_rows_ms{0.0};
  double scan_cells_ms{0.0};
  double infer_columns_ms{0.0};
};

struct CsvLoadProfile {
  double detect_sep_ms{0.0};
  double parse_records_ms{0.0};
  double build_dataframe_ms{0.0};
  double total_ms{0.0};
  std::size_t total_rows{0};
  std::size_t selected_rows{0};
  std::size_t columns{0};
};

struct ColumnarRawResult {
  CsvRawBatch batch{};
  std::size_t parsed_rows{0};
};

struct TextArenaBuilder {
  std::vector<char> arena{};
  std::vector<std::pair<std::size_t, std::size_t>> spans{};

  void reserve(std::size_t rows_hint) {
    arena.reserve(rows_hint * 16);
    spans.reserve(rows_hint);
  }

  void append(std::string_view token) {
    const auto off = arena.size();
    arena.insert(arena.end(), token.begin(), token.end());
    spans.push_back(std::pair<std::size_t, std::size_t>{off, token.size()});
  }

  pluss::coden::Note materialize_note() {
    const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(spans.size())};
    return pluss::coden::ops_internal::note_from_utf8_packed(
        std::move(arena),
        std::move(spans),
        std::span<const std::int64_t>(shape.data(), shape.size()));
  }
};

std::string lower_copy(std::string s) {
  for (auto& ch : s) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return s;
}

bool series_value_is_nan_for_io(const pluss::Series& s, std::int64_t row) {
  const auto st = s.scalar_type();
  const auto* base = s.raw_data();
  switch (st) {
    case pluss::codem::ScalarType::Float16:
      return std::isnan(
          pluss::codem::float16_to_float(reinterpret_cast<const pluss::codem::Float16*>(base)[row]));
    case pluss::codem::ScalarType::BFloat16:
      return std::isnan(
          pluss::codem::bfloat16_to_float(reinterpret_cast<const pluss::codem::BFloat16*>(base)[row]));
    case pluss::codem::ScalarType::Float32:
      return std::isnan(reinterpret_cast<const float*>(base)[row]);
    case pluss::codem::ScalarType::Float64:
      return std::isnan(reinterpret_cast<const double*>(base)[row]);
    case pluss::codem::ScalarType::Complex64: {
      const auto v = reinterpret_cast<const pluss::codem::Complex64*>(base)[row];
      return std::isnan(v.real()) || std::isnan(v.imag());
    }
    case pluss::codem::ScalarType::Complex128: {
      const auto v = reinterpret_cast<const pluss::codem::Complex128*>(base)[row];
      return std::isnan(v.real()) || std::isnan(v.imag());
    }
    default:
      return false;
  }
}

bool slate_value_is_missing_for_io(const pluss::table::Slate& t, std::int64_t row) {
  if (pluss::table::detail::has_validity_mask(t)) {
    const auto valid = pluss::table::detail::validity_mask(t)->to_vec<bool>();
    if (!valid[static_cast<std::size_t>(row)]) {
      return true;
    }
  }
  if (t.kind() == pluss::table::SlateKind::Series) {
    return series_value_is_nan_for_io(t.as_series(), row);
  }
  if (t.kind() == pluss::table::SlateKind::Duration) {
    return t.as_duration().host()[static_cast<std::size_t>(row)] ==
           pluss::Duration::kNaT;
  }
  return false;
}

bool series_all_zero_i64_for_io(const pluss::Series& s) {
  const auto values = s.to_vec<std::int64_t>();
  return std::all_of(values.begin(), values.end(), [](const std::int64_t v) { return v == 0; });
}

pluss::table::Slate temporal_slate_for_io(const pluss::table::Slate& t) {
  const auto timeline = t.as_timeline();
  const auto has_subsecond = !series_all_zero_i64_for_io(timeline.nanosecond());
  const auto has_clock_time = has_subsecond || !series_all_zero_i64_for_io(timeline.second())
                              || !series_all_zero_i64_for_io(timeline.minute())
                              || !series_all_zero_i64_for_io(timeline.hour());
  if (!has_clock_time) {
    return pluss::table::Slate(timeline.strftime("%Y-%m-%d"), pluss::table::detail::validity_mask(t));
  }
  if (!has_subsecond) {
    return pluss::table::Slate(timeline.strftime("%Y-%m-%d %H:%M:%S"), pluss::table::detail::validity_mask(t));
  }
  return pluss::table::Slate(timeline.strftime("%Y-%m-%d %H:%M:%S.%f"), pluss::table::detail::validity_mask(t));
}

pluss::table::Slate slate_for_csv_io(const pluss::table::Slate& t) {
  if (t.kind() == pluss::table::SlateKind::Timeline) {
    return temporal_slate_for_io(t);
  }
  return t;
}

bool csv_profile_enabled(const ReadCsvOptions& options) {
  if (options.profile) {
    return true;
  }
  if (const char* env = std::getenv("PLUSSMIC_CSV_PROFILE")) {
    const auto flag = lower_copy(std::string(env));
    return flag == "1" || flag == "true" || flag == "on" || flag == "yes";
  }
  return false;
}

double elapsed_ms(std::chrono::steady_clock::time_point start,
                  std::chrono::steady_clock::time_point stop) {
  return std::chrono::duration<double, std::milli>(stop - start).count();
}

void ltrim_inplace(std::string& s) {
  std::size_t i = 0;
  while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
    ++i;
  }
  if (i > 0) {
    s.erase(0, i);
  }
}

void strip_utf8_bom_inplace(std::string& s) {
  constexpr std::string_view kUtf8Bom{"\xEF\xBB\xBF", 3};
  if (s.starts_with(kUtf8Bom)) {
    s.erase(0, kUtf8Bom.size());
  }
}

char effective_separator(const ReadCsvOptions& options) {
  if (options.delimiter.has_value()) {
    return *options.delimiter;
  }
  return options.sep;
}

char sniff_separator_from_sample(std::string_view sample, std::optional<char> quotechar, char fallback) {
  constexpr std::array<char, 4> kCandidates{',', ';', '\t', '|'};
  char best = fallback;
  double best_score = -std::numeric_limits<double>::infinity();

  for (const char candidate : kCandidates) {
    std::size_t lines = 0;
    std::size_t non_zero_lines = 0;
    std::size_t total_hits = 0;
    std::size_t current_hits = 0;
    bool in_quotes = false;

    for (std::size_t i = 0; i < sample.size(); ++i) {
      const char ch = sample[i];
      if (quotechar.has_value() && ch == *quotechar) {
        if (i + 1 < sample.size() && quotechar.has_value() && sample[i + 1] == *quotechar) {
          ++i;
        } else {
          in_quotes = !in_quotes;
        }
        continue;
      }

      if (!in_quotes && ch == candidate) {
        ++current_hits;
      }

      if (ch == '\n') {
        ++lines;
        total_hits += current_hits;
        if (current_hits > 0) {
          ++non_zero_lines;
        }
        current_hits = 0;
      }
    }

    if (current_hits > 0 || !sample.empty()) {
      ++lines;
      total_hits += current_hits;
      if (current_hits > 0) {
        ++non_zero_lines;
      }
    }

    if (lines == 0 || non_zero_lines == 0) {
      continue;
    }

    const double avg_hits = static_cast<double>(total_hits) / static_cast<double>(lines);
    const double coverage = static_cast<double>(non_zero_lines) / static_cast<double>(lines);
    const double score = avg_hits * 100.0 + coverage * 10.0;
    if (score > best_score) {
      best_score = score;
      best = candidate;
    }
  }

  return best;
}

char detect_separator(const ReadCsvOptions& options) {
  if (options.delimiter.has_value()) {
    return *options.delimiter;
  }
  if (options.sep.explicit_set) {
    return options.sep;
  }
  if (!options.auto_detect_sep) {
    return options.sep;
  }

  std::ifstream in(options.filepath, std::ios::in | std::ios::binary);
  if (!in.is_open()) {
    return options.sep;
  }

  constexpr std::size_t kSampleBytes = 64 * 1024;
  std::string sample(kSampleBytes, '\0');
  in.read(sample.data(), static_cast<std::streamsize>(sample.size()));
  sample.resize(static_cast<std::size_t>(in.gcount()));
  if (sample.empty()) {
    return options.sep;
  }

  return sniff_separator_from_sample(sample, options.quotechar, options.sep);
}

bool parse_record(std::istream& in, const ReadCsvOptions& options, std::vector<std::string>& out) {
  out.clear();
  std::string field;
  const auto sep = effective_separator(options);
  const auto quote = options.quotechar;

  bool in_quotes = false;
  bool any_content = false;

  while (true) {
    const int ci = in.get();
    if (ci == EOF) {
      if (in_quotes) {
        PLUSS_FAIL("load_csv: unterminated quoted field at EOF");
      }
      if (!any_content && field.empty() && out.empty()) {
        return false;
      }
      out.push_back(std::move(field));
      return true;
    }

    char c = static_cast<char>(ci);

    if (!in_quotes) {
      if (c == '\r') {
        continue;
      }
      if (out.empty() && field.empty() && options.comment.has_value() && c == *options.comment) {
        while (true) {
          const int cj = in.get();
          if (cj == EOF || static_cast<char>(cj) == '\n') {
            break;
          }
        }
        if (options.skip_blank_lines) {
          field.clear();
          out.clear();
          any_content = false;
          continue;
        }
        out.push_back("");
        return true;
      }
      if (quote.has_value() && c == *quote) {
        in_quotes = true;
        any_content = true;
        continue;
      }
      if (c == sep) {
        out.push_back(std::move(field));
        field.clear();
        any_content = true;
        continue;
      }
      if (c == '\n') {
        if (!any_content && field.empty() && out.empty() && options.skip_blank_lines) {
          continue;
        }
        out.push_back(std::move(field));
        return true;
      }
      field.push_back(c);
      any_content = true;
    } else {
      if (options.escapechar.has_value() && c == *options.escapechar) {
        const int next = in.get();
        if (next == EOF) {
          PLUSS_FAIL("load_csv: dangling escape at EOF");
        }
        field.push_back(static_cast<char>(next));
        continue;
      }
      if (quote.has_value() && c == *quote) {
        if (options.doublequote) {
          const int pk = in.peek();
          if (pk != EOF && static_cast<char>(pk) == *quote) {
            in.get();
            field.push_back(*quote);
            continue;
          }
        }
        in_quotes = false;
        continue;
      }
      field.push_back(c);
      any_content = true;
    }
  }
}

std::vector<std::size_t> resolve_usefeats(const std::vector<std::string>& names,
                                          const ReadCsvOptions& options) {
  std::vector<std::size_t> out;

  if (options.usefeats.empty() && options.usefeats_idx.empty()) {
    out.resize(names.size());
    for (std::size_t i = 0; i < names.size(); ++i) {
      out[i] = i;
    }
    return out;
  }

  for (const auto idx : options.usefeats_idx) {
    PLUSS_CHECK(idx < names.size(), fmt::format("load_csv: usefeats_idx {} out of range", idx));
    out.push_back(idx);
  }

  for (const auto& name : options.usefeats) {
    const auto it = std::find(names.begin(), names.end(), name);
    PLUSS_CHECK(it != names.end(), "load_csv: unknown usefeats name '" + name + "'");
    out.push_back(static_cast<std::size_t>(std::distance(names.begin(), it)));
  }

  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return out;
}

std::string scalar_for_csv(const pluss::Series& s, std::int64_t row, const WriteCsvOptions& options) {
  const auto st = s.scalar_type();
  const auto* base = s.raw_data();
  auto maybe_replace_decimal = [&](std::string out) {
    if (options.decimal != '.') {
      std::replace(out.begin(), out.end(), '.', options.decimal);
    }
    return out;
  };
  switch (st) {
    case pluss::codem::ScalarType::Bool:
      return reinterpret_cast<const bool*>(base)[row] ? "true" : "false";
    case pluss::codem::ScalarType::Int8:
      return fmt::format("{}", static_cast<int>(reinterpret_cast<const std::int8_t*>(base)[row]));
    case pluss::codem::ScalarType::Int16:
      return fmt::format("{}", reinterpret_cast<const std::int16_t*>(base)[row]);
    case pluss::codem::ScalarType::Int32:
      return fmt::format("{}", reinterpret_cast<const std::int32_t*>(base)[row]);
    case pluss::codem::ScalarType::Int64:
      return fmt::format("{}", reinterpret_cast<const std::int64_t*>(base)[row]);
    case pluss::codem::ScalarType::UInt8:
      return fmt::format("{}", static_cast<unsigned>(reinterpret_cast<const std::uint8_t*>(base)[row]));
    case pluss::codem::ScalarType::UInt16:
      return fmt::format("{}", reinterpret_cast<const std::uint16_t*>(base)[row]);
    case pluss::codem::ScalarType::UInt32:
      return fmt::format("{}", reinterpret_cast<const std::uint32_t*>(base)[row]);
    case pluss::codem::ScalarType::UInt64:
      return fmt::format("{}", reinterpret_cast<const std::uint64_t*>(base)[row]);
    case pluss::codem::ScalarType::Float16:
      return maybe_replace_decimal(fmt::format("{:.6g}",
                                               pluss::codem::float16_to_float(reinterpret_cast<const pluss::codem::Float16*>(base)[row])));
    case pluss::codem::ScalarType::BFloat16:
      return maybe_replace_decimal(fmt::format("{:.6g}",
                                               pluss::codem::bfloat16_to_float(reinterpret_cast<const pluss::codem::BFloat16*>(base)[row])));
    case pluss::codem::ScalarType::Float32:
    {
      const auto v = reinterpret_cast<const float*>(base)[row];
      if (options.float_format.has_value()) {
        return maybe_replace_decimal(fmt::format(fmt::runtime(*options.float_format), v));
      }
      return maybe_replace_decimal(fmt::format("{:.8g}", v));
    }
    case pluss::codem::ScalarType::Float64:
    {
      const auto v = reinterpret_cast<const double*>(base)[row];
      if (options.float_format.has_value()) {
        return maybe_replace_decimal(fmt::format(fmt::runtime(*options.float_format), v));
      }
      return maybe_replace_decimal(fmt::format("{:.12g}", v));
    }
    case pluss::codem::ScalarType::Complex64: {
      const auto v = reinterpret_cast<const pluss::codem::Complex64*>(base)[row];
      return fmt::format("({:.6g}{:+.6g}j)", v.real(), v.imag());
    }
    case pluss::codem::ScalarType::Complex128: {
      const auto v = reinterpret_cast<const pluss::codem::Complex128*>(base)[row];
      return fmt::format("({:.6g}{:+.6g}j)", v.real(), v.imag());
    }
    default:
      return "";
  }
}

std::string slate_value_for_csv(const pluss::table::Slate& t, std::int64_t row, const WriteCsvOptions& options) {
  if (slate_value_is_missing_for_io(t, row)) {
    return options.null_rep;
  }

  switch (t.kind()) {
    case pluss::table::SlateKind::Series:
      return scalar_for_csv(t.as_series(), row, options);
    case pluss::table::SlateKind::Note:
      return std::string(t.as_note().at(row));
    case pluss::table::SlateKind::Timeline:
      return fmt::format("{}", t.as_timeline().ticks()[static_cast<std::size_t>(row)]);
    case pluss::table::SlateKind::Duration:
      return fmt::format("{}", t.as_duration().host()[static_cast<std::size_t>(row)]);
  }
  PLUSS_FAIL("write_csv: unsupported Slate kind");
}

std::string escape_csv_cell(std::string_view value, const WriteCsvOptions& options) {
  const auto sep = options.sep;
  const auto quotechar = options.quotechar;
  bool needs_quotes = false;
  for (const char ch : value) {
    if (ch == sep || ch == quotechar || ch == '\n' || ch == '\r') {
      needs_quotes = true;
      break;
    }
  }
  if (!needs_quotes) {
    return std::string(value);
  }

  std::string out;
  out.reserve(value.size() + 4);
  out.push_back(quotechar);
  for (const char ch : value) {
    if (ch == quotechar) {
      if (options.doublequote) {
        out.push_back(quotechar);
      } else {
        out.push_back('\\');
      }
    }
    out.push_back(ch);
  }
  out.push_back(quotechar);
  return out;
}

enum class RuntimeCsvEngine : std::uint8_t {
  Parallel = 0,
  Serial = 1,
};

RuntimeCsvEngine resolve_runtime_engine(const ReadCsvOptions& options, std::size_t file_bytes) {
  if (options.engine == CsvEngine::Parallel) {
    return RuntimeCsvEngine::Parallel;
  }
  if (options.engine == CsvEngine::Serial) {
    return RuntimeCsvEngine::Serial;
  }

  // Auto policy:
  // - small explicit row windows should favor parallel for latency.
  // - very large files should favor serial (Note-backed) ingest for memory stability.
  //   default threshold is 2 GiB (override via autoserialminbytes config).
  if (options.ncases.has_value() && *options.ncases <= 250000) {
    return RuntimeCsvEngine::Parallel;
  }
  if (file_bytes >= options.auto_serial_min_bytes) {
    return RuntimeCsvEngine::Serial;
  }
  return RuntimeCsvEngine::Parallel;
}

std::optional<ColumnarRawResult> try_build_raw_batch_serial(const ReadCsvOptions& options,
                                                            CsvRecordBuildProfile* profile) {
  if (options.skipfooter > 0) {
    return std::nullopt;
  }

  std::ifstream in(options.filepath, std::ios::in | std::ios::binary);
  PLUSS_CHECK(in.is_open(), "load_csv: failed to open file '" + options.filepath + "'");

  if (options.header) {
    PLUSS_CHECK(options.header_row >= 0, "load_csv: header_row must be >= 0");
  }

  const auto t_scan_start = std::chrono::steady_clock::now();

  std::vector<std::string> row;
  row.reserve(64);

  std::vector<std::string> names;
  std::vector<std::size_t> selected_cols;
  std::vector<std::string> selected_names;
  std::optional<std::size_t> case_col;
  std::vector<TextArenaBuilder> values_by_col;
  TextArenaBuilder case_values;

  std::size_t parsed_rows = 0;
  std::size_t kept_rows = 0;
  std::size_t data_rows_seen = 0;
  std::size_t bad_lines = 0;
  std::size_t accepted_rows = 0;
  bool layout_initialized = false;

  auto push_cell = [&](TextArenaBuilder& out, const std::string& src) {
    if (!options.skipinitialspace) {
      out.append(src);
      return;
    }
    if (!src.empty() && std::isspace(static_cast<unsigned char>(src.front()))) {
      auto cell = src;
      ltrim_inplace(cell);
      out.append(cell);
      return;
    }
    out.append(src);
  };

  auto init_layout = [&](std::size_t first_row_ncol, const std::vector<std::string>* header_row) {
    if (header_row != nullptr) {
      names = *header_row;
      if (options.skipinitialspace) {
        for (auto& n : names) {
          ltrim_inplace(n);
        }
      }
    } else {
      names.clear();
      names.reserve(first_row_ncol);
      for (std::size_t i = 0; i < first_row_ncol; ++i) {
        names.push_back(fmt::format("feat_{}", i));
      }
    }

    if (!options.names.empty()) {
      names = options.names;
    }
    PLUSS_CHECK(!names.empty(), "load_csv: inferred zero columns");

    const auto usefeats = resolve_usefeats(names, options);

    if (options.case_col_idx.has_value()) {
      PLUSS_CHECK(*options.case_col_idx < names.size(), "load_csv: case_col_idx is out of range");
      case_col = *options.case_col_idx;
    }
    if (options.case_col.has_value()) {
      const auto it = std::find(names.begin(), names.end(), *options.case_col);
      PLUSS_CHECK(it != names.end(), "load_csv: unknown case_col name '" + *options.case_col + "'");
      case_col = static_cast<std::size_t>(std::distance(names.begin(), it));
    }

    selected_cols.clear();
    selected_cols.reserve(usefeats.size());
    for (const auto c : usefeats) {
      if (case_col.has_value() && c == *case_col) {
        continue;
      }
      selected_cols.push_back(c);
    }

    selected_names.clear();
    selected_names.reserve(selected_cols.size());
    for (const auto c : selected_cols) {
      selected_names.push_back(names[c]);
    }

    const auto reserve_rows = options.ncases.value_or(static_cast<std::size_t>(4096));
    values_by_col.assign(selected_cols.size(), {});
    for (auto& col : values_by_col) {
      col.reserve(reserve_rows);
    }
    if (case_col.has_value()) {
      case_values.reserve(reserve_rows);
    }
  };

  while (parse_record(in, options, row)) {
    if (parsed_rows == 0 && !row.empty()) {
      strip_utf8_bom_inplace(row.front());
    }
    const auto logical_row = parsed_rows++;
    if (options.skipcases_first_n > 0 && logical_row < options.skipcases_first_n) {
      continue;
    }
    if (options.skipcases.find(logical_row) != options.skipcases.end()) {
      continue;
    }

    const auto kept_pos = kept_rows++;

    if (!layout_initialized) {
      if (options.header) {
        const auto header_pos = static_cast<std::size_t>(options.header_row);
        if (kept_pos < header_pos) {
          continue;
        }
        if (kept_pos == header_pos) {
          init_layout(row.size(), &row);
          layout_initialized = true;
          continue;
        }
      } else {
        init_layout(row.size(), nullptr);
        layout_initialized = true;
      }
    }

    ++data_rows_seen;

    if (row.size() != names.size()) {
      ++bad_lines;
      if (options.on_bad_lines == BadLinePolicy::Error) {
        PLUSS_FAIL(fmt::format("load_csv: bad line at data row {} (expected {} fields, got {})",
                               data_rows_seen - 1,
                               names.size(),
                               row.size()));
      }
      if (options.on_bad_lines == BadLinePolicy::Warn) {
        ::pluss::codem::warn(fmt::format("load_csv: skipping malformed row {}", data_rows_seen - 1));
      }
      continue;
    }

    if (options.ncases.has_value() && accepted_rows >= *options.ncases) {
      break;
    }

    for (std::size_t i = 0; i < selected_cols.size(); ++i) {
      push_cell(values_by_col[i], row[selected_cols[i]]);
    }
    if (case_col.has_value()) {
      push_cell(case_values, row[*case_col]);
    }

    ++accepted_rows;
  }

  if (profile != nullptr) {
    profile->filter_rows_ms = 0.0;
    profile->scan_cells_ms = elapsed_ms(t_scan_start, std::chrono::steady_clock::now());
  }

  PLUSS_CHECK(kept_rows > 0, "load_csv: no rows parsed from csv");
  if (options.header) {
    PLUSS_CHECK(layout_initialized, "load_csv: header_row is out of range");
  }

  if (options.on_bad_lines == BadLinePolicy::Warn && bad_lines > 0) {
    ::pluss::codem::warn(fmt::format("load_csv: skipped {} malformed rows", bad_lines));
  }

  ColumnarRawResult result;
  result.parsed_rows = parsed_rows;
  result.batch.features.reserve(selected_cols.size());
  for (std::size_t i = 0; i < selected_cols.size(); ++i) {
    result.batch.features.push_back(CsvRawColumn{
        selected_names[i],
        selected_cols[i],
        {},
        std::make_optional(values_by_col[i].materialize_note()),
    });
  }
  if (case_col.has_value()) {
    result.batch.cases = CsvRawColumn{
        names[*case_col],
        *case_col,
        {},
        std::make_optional(case_values.materialize_note()),
    };
  }

  return result;
}

CsvRawBatch build_raw_batch_from_records(const std::vector<std::vector<std::string>>& records,
                                         const ReadCsvOptions& options,
                                         CsvRecordBuildProfile* profile) {
  const auto t_filter_start = std::chrono::steady_clock::now();
  std::vector<std::size_t> kept_rows;
  kept_rows.reserve(records.size());
  for (std::size_t logical_row = 0; logical_row < records.size(); ++logical_row) {
    if (options.skipcases_first_n > 0 && logical_row < options.skipcases_first_n) {
      continue;
    }
    if (options.skipcases.find(logical_row) != options.skipcases.end()) {
      continue;
    }
    kept_rows.push_back(logical_row);
  }
  if (profile != nullptr) {
    profile->filter_rows_ms = elapsed_ms(t_filter_start, std::chrono::steady_clock::now());
  }

  PLUSS_CHECK(!kept_rows.empty(), "load_csv: no rows parsed from csv");

  std::vector<std::string> names;
  std::size_t data_start_pos = 0;

  if (options.header) {
    PLUSS_CHECK(options.header_row >= 0, "load_csv: header_row must be >= 0");
    const auto header_pos = static_cast<std::size_t>(options.header_row);
    PLUSS_CHECK(header_pos < kept_rows.size(), "load_csv: header_row is out of range");
    names = records[kept_rows[header_pos]];
    if (!names.empty()) {
      strip_utf8_bom_inplace(names.front());
    }
    if (options.skipinitialspace) {
      for (auto& n : names) {
        ltrim_inplace(n);
      }
    }
    data_start_pos = header_pos + 1;
  } else {
    const auto ncol = records[kept_rows.front()].size();
    names.reserve(ncol);
    for (std::size_t i = 0; i < ncol; ++i) {
      names.push_back(fmt::format("feat_{}", i));
    }
    data_start_pos = 0;
  }

  if (!options.names.empty()) {
    names = options.names;
  }

  PLUSS_CHECK(!names.empty(), "load_csv: inferred zero columns");

  if (options.skipfooter > 0) {
    const auto n_data = kept_rows.size() > data_start_pos ? (kept_rows.size() - data_start_pos) : 0;
    PLUSS_CHECK(options.skipfooter <= n_data, "load_csv: skipfooter exceeds number of data rows");
  }

  const auto usefeats = resolve_usefeats(names, options);

  std::optional<std::size_t> case_col;
  if (options.case_col_idx.has_value()) {
    PLUSS_CHECK(*options.case_col_idx < names.size(), "load_csv: case_col_idx is out of range");
    case_col = *options.case_col_idx;
  }
  if (options.case_col.has_value()) {
    const auto it = std::find(names.begin(), names.end(), *options.case_col);
    PLUSS_CHECK(it != names.end(), "load_csv: unknown case_col name '" + *options.case_col + "'");
    case_col = static_cast<std::size_t>(std::distance(names.begin(), it));
  }

  std::vector<std::size_t> selected_cols;
  selected_cols.reserve(usefeats.size());
  for (const auto c : usefeats) {
    if (case_col.has_value() && c == *case_col) {
      continue;
    }
    selected_cols.push_back(c);
  }

  std::vector<std::string> selected_names;
  selected_names.reserve(selected_cols.size());
  for (const auto c : selected_cols) {
    selected_names.push_back(names[c]);
  }

  const auto data_end_pos = kept_rows.size() - options.skipfooter;
  const auto raw_data_rows = data_end_pos > data_start_pos ? (data_end_pos - data_start_pos) : 0;
  const auto reserve_rows = options.ncases.has_value()
      ? std::min<std::size_t>(raw_data_rows, *options.ncases)
      : raw_data_rows;

  const auto t_scan_start = std::chrono::steady_clock::now();
  std::vector<TextArenaBuilder> values_by_col(selected_cols.size());
  for (auto& values : values_by_col) {
    values.reserve(reserve_rows);
  }
  TextArenaBuilder case_values;
  if (case_col.has_value()) {
    case_values.reserve(reserve_rows);
  }

  std::size_t bad_lines = 0;
  std::size_t accepted_rows = 0;

  auto push_cell = [&](TextArenaBuilder& out, const std::string& src) {
    if (!options.skipinitialspace) {
      out.append(src);
      return;
    }
    if (!src.empty() && std::isspace(static_cast<unsigned char>(src.front()))) {
      auto cell = src;
      ltrim_inplace(cell);
      out.append(cell);
      return;
    }
    out.append(src);
  };

  for (std::size_t pos = data_start_pos; pos < data_end_pos; ++pos) {
    const auto row_idx = kept_rows[pos];
    const auto& rec = records[row_idx];

    if (rec.size() != names.size()) {
      ++bad_lines;
      if (options.on_bad_lines == BadLinePolicy::Error) {
        PLUSS_FAIL(fmt::format("load_csv: bad line at data row {} (expected {} fields, got {})",
                               pos - data_start_pos,
                               names.size(),
                               rec.size()));
      }
      if (options.on_bad_lines == BadLinePolicy::Warn) {
        ::pluss::codem::warn(fmt::format("load_csv: skipping malformed row {}", pos - data_start_pos));
      }
      continue;
    }

    if (options.ncases.has_value() && accepted_rows >= *options.ncases) {
      break;
    }

    for (std::size_t i = 0; i < selected_cols.size(); ++i) {
      push_cell(values_by_col[i], rec[selected_cols[i]]);
    }

    if (case_col.has_value()) {
      push_cell(case_values, rec[*case_col]);
    }

    ++accepted_rows;
  }

  if (profile != nullptr) {
    profile->scan_cells_ms = elapsed_ms(t_scan_start, std::chrono::steady_clock::now());
  }

  if (options.on_bad_lines == BadLinePolicy::Warn && bad_lines > 0) {
    ::pluss::codem::warn(fmt::format("load_csv: skipped {} malformed rows", bad_lines));
  }

  CsvRawBatch out;
  out.features.reserve(selected_cols.size());
  for (std::size_t i = 0; i < selected_cols.size(); ++i) {
    out.features.push_back(CsvRawColumn{
        selected_names[i],
        selected_cols[i],
        {},
        std::make_optional(values_by_col[i].materialize_note()),
    });
  }
  if (case_col.has_value()) {
    out.cases = CsvRawColumn{
        names[*case_col],
        *case_col,
        {},
        std::make_optional(case_values.materialize_note()),
    };
  }
  return out;
}

CsvBatch build_batch_from_raw(const CsvRawBatch& raw,
                              const ReadCsvOptions& options,
                              CsvRecordBuildProfile* profile) {
  double infer_ms = 0.0;
  auto out = pluss::table::detail::csv_batch_from_raw_batch(raw, options, &infer_ms);
  if (profile != nullptr) {
    profile->infer_columns_ms = infer_ms;
  }
  return out;
}

CsvBatch build_batch_from_records(const std::vector<std::vector<std::string>>& records,
                                  const ReadCsvOptions& options,
                                  CsvRecordBuildProfile* profile) {
  const auto raw = build_raw_batch_from_records(records, options, profile);
  return build_batch_from_raw(raw, options, profile);
}

} // namespace

CsvRawBatch read_csv_records_raw_batch(const std::vector<std::vector<std::string>>& records,
                                       const ReadCsvOptions& options) {
  PLUSS_CHECK(
      !options.chunksize.has_value(),
      "read_csv_records_raw_batch: chunksize is only valid for CsvCaseReader");
  return build_raw_batch_from_records(records, options, nullptr);
}

CsvRawBatch read_csv_raw_batch(const ReadCsvOptions& options) {
  PLUSS_CHECK(
      !options.chunksize.has_value(),
      "load_csv: chunksize requires CsvCaseReader because a DataFrame return "
      "cannot represent several streamed batches");
  ReadCsvOptions resolved = options;
  auto source = detail::resolve_local_or_http_source(resolved.filepath, "load_csv");
  resolved.filepath = source.local_path();

  const auto t_total_start = std::chrono::steady_clock::now();
  CsvLoadProfile profile_stats;

  {
    const auto t0 = std::chrono::steady_clock::now();
    resolved.sep = detect_separator(resolved);
    profile_stats.detect_sep_ms = elapsed_ms(t0, std::chrono::steady_clock::now());
  }

  std::error_code fs_ec;
  const auto file_bytes = std::filesystem::file_size(resolved.filepath, fs_ec);
  const auto known_size = fs_ec ? static_cast<std::size_t>(0) : static_cast<std::size_t>(file_bytes);
  const auto runtime_engine = resolve_runtime_engine(resolved, known_size);
  const bool explicit_parallel = resolved.engine == CsvEngine::Parallel;

  CsvRecordBuildProfile build_profile;

  std::optional<ColumnarRawResult> serial_raw = std::nullopt;
  const auto t_parse_start = std::chrono::steady_clock::now();
  if (runtime_engine == RuntimeCsvEngine::Serial) {
    serial_raw = try_build_raw_batch_serial(resolved, &build_profile);
    if (!serial_raw.has_value()) {
      ::pluss::codem::warn(
          fmt::format("load_csv: serial parser fallback to row parser for '{}'", resolved.filepath));
    }
  }

  CsvRawBatch out;
  if (serial_raw.has_value()) {
    profile_stats.parse_records_ms = elapsed_ms(t_parse_start, std::chrono::steady_clock::now());
    out = std::move(serial_raw->batch);
    profile_stats.total_rows = serial_raw->parsed_rows;
    profile_stats.build_dataframe_ms = 0.0;
  } else {
    std::vector<std::vector<std::string>> records;
    auto parse_serial = [&]() {
      std::ifstream in(resolved.filepath, std::ios::in | std::ios::binary);
      PLUSS_CHECK(in.is_open(), "load_csv: failed to open file '" + resolved.filepath + "'");
      std::vector<std::vector<std::string>> out_rows;
      out_rows.reserve(4096);
      std::vector<std::string> row;
      while (parse_record(in, resolved, row)) {
        out_rows.push_back(row);
      }
      return out_rows;
    };

    const bool attempt_parallel =
        runtime_engine == RuntimeCsvEngine::Parallel || known_size >= resolved.parallel_min_bytes;
    if (attempt_parallel) {
      try {
        records = ParallelCsvReader::read_file(resolved);
      } catch (const std::exception& e) {
        if (explicit_parallel) {
          throw;
        }
        ::pluss::codem::warn(
            fmt::format("load_csv: parallel parse fallback to serial for '{}' ({})", resolved.filepath, e.what()));
        records = parse_serial();
      }
    } else {
      records = parse_serial();
    }
    profile_stats.parse_records_ms = elapsed_ms(t_parse_start, std::chrono::steady_clock::now());
    profile_stats.total_rows = records.size();

    const auto t_build_start = std::chrono::steady_clock::now();
    out = build_raw_batch_from_records(records, resolved, &build_profile);
    profile_stats.build_dataframe_ms = elapsed_ms(t_build_start, std::chrono::steady_clock::now());
  }

  profile_stats.total_ms = elapsed_ms(t_total_start, std::chrono::steady_clock::now());
  auto raw_col_size = [](const CsvRawColumn& col) -> std::size_t {
    return col.note_values.has_value() ? static_cast<std::size_t>(col.note_values->numel()) : col.values.size();
  };
  profile_stats.selected_rows = out.features.empty()
      ? (out.cases.has_value() ? raw_col_size(*out.cases) : 0u)
      : raw_col_size(out.features.front());
  profile_stats.columns = out.features.size();

  if (csv_profile_enabled(resolved)) {
    fmt::print(
        stderr,
        "load_csv(raw) profile [{}]: total={:.2f} ms, detect_sep={:.2f} ms, parse_records={:.2f} ms, "
        "build_batch={:.2f} ms, case_filter={:.2f} ms, scan_cells={:.2f} ms, "
        "cases(raw/selected)={}/{}, features={}\n",
        resolved.filepath,
        profile_stats.total_ms,
        profile_stats.detect_sep_ms,
        profile_stats.parse_records_ms,
        profile_stats.build_dataframe_ms,
        build_profile.filter_rows_ms,
        build_profile.scan_cells_ms,
        profile_stats.total_rows,
        profile_stats.selected_rows,
        profile_stats.columns);
  }

  return out;
}

CsvRawBatch read_csv_raw_batch(std::string_view filepath, const ReadCsvOptions& options) {
  ReadCsvOptions resolved = options;
  resolved.filepath = std::string(filepath);
  return read_csv_raw_batch(resolved);
}

CsvBatch read_csv_records_batch(const std::vector<std::vector<std::string>>& records,
                                const ReadCsvOptions& options) {
  PLUSS_CHECK(
      !options.chunksize.has_value(),
      "read_csv_records_batch: chunksize is only valid for CsvCaseReader");
  return build_batch_from_records(records, options, nullptr);
}

CsvBatch read_csv_batch(const ReadCsvOptions& options) {
  PLUSS_CHECK(
      !options.chunksize.has_value(),
      "load_csv: chunksize requires CsvCaseReader because a DataFrame return "
      "cannot represent several streamed batches");
  ReadCsvOptions resolved = options;
  auto source = detail::resolve_local_or_http_source(resolved.filepath, "load_csv");
  resolved.filepath = source.local_path();

  const auto t_total_start = std::chrono::steady_clock::now();
  CsvLoadProfile profile_stats;

  {
    const auto t0 = std::chrono::steady_clock::now();
    resolved.sep = detect_separator(resolved);
    profile_stats.detect_sep_ms = elapsed_ms(t0, std::chrono::steady_clock::now());
  }

  std::error_code fs_ec;
  const auto file_bytes = std::filesystem::file_size(resolved.filepath, fs_ec);
  const auto known_size = fs_ec ? static_cast<std::size_t>(0) : static_cast<std::size_t>(file_bytes);
  const auto runtime_engine = resolve_runtime_engine(resolved, known_size);
  const bool explicit_parallel = resolved.engine == CsvEngine::Parallel;

  CsvRecordBuildProfile build_profile;

  std::optional<ColumnarRawResult> serial_raw = std::nullopt;
  const auto t_parse_start = std::chrono::steady_clock::now();
  if (runtime_engine == RuntimeCsvEngine::Serial) {
    serial_raw = try_build_raw_batch_serial(resolved, &build_profile);
    if (!serial_raw.has_value()) {
      ::pluss::codem::warn(
          fmt::format("load_csv: serial parser fallback to row parser for '{}'", resolved.filepath));
    }
  }

  CsvBatch out;
  if (serial_raw.has_value()) {
    profile_stats.parse_records_ms = elapsed_ms(t_parse_start, std::chrono::steady_clock::now());
    profile_stats.total_rows = serial_raw->parsed_rows;
    const auto t_build_start = std::chrono::steady_clock::now();
    out = build_batch_from_raw(serial_raw->batch, resolved, &build_profile);
    profile_stats.build_dataframe_ms = elapsed_ms(t_build_start, std::chrono::steady_clock::now());
  } else {
    std::vector<std::vector<std::string>> records;
    auto parse_serial = [&]() {
      std::ifstream in(resolved.filepath, std::ios::in | std::ios::binary);
      PLUSS_CHECK(in.is_open(), "load_csv: failed to open file '" + resolved.filepath + "'");
      std::vector<std::vector<std::string>> out_rows;
      out_rows.reserve(4096);
      std::vector<std::string> row;
      while (parse_record(in, resolved, row)) {
        out_rows.push_back(row);
      }
      return out_rows;
    };

    const bool attempt_parallel =
        runtime_engine == RuntimeCsvEngine::Parallel || known_size >= resolved.parallel_min_bytes;
    if (attempt_parallel) {
      try {
        records = ParallelCsvReader::read_file(resolved);
      } catch (const std::exception& e) {
        if (explicit_parallel) {
          throw;
        }
        ::pluss::codem::warn(
            fmt::format("load_csv: parallel parse fallback to serial for '{}' ({})", resolved.filepath, e.what()));
        records = parse_serial();
      }
    } else {
      records = parse_serial();
    }
    profile_stats.parse_records_ms = elapsed_ms(t_parse_start, std::chrono::steady_clock::now());
    profile_stats.total_rows = records.size();

    const auto t_build_start = std::chrono::steady_clock::now();
    out = build_batch_from_records(records, resolved, &build_profile);
    profile_stats.build_dataframe_ms = elapsed_ms(t_build_start, std::chrono::steady_clock::now());
  }

  profile_stats.total_ms = elapsed_ms(t_total_start, std::chrono::steady_clock::now());
  profile_stats.selected_rows = out.features.empty()
      ? (out.cases.has_value() ? static_cast<std::size_t>(out.cases->size()) : 0u)
      : static_cast<std::size_t>(out.features.front().second.size());
  profile_stats.columns = out.features.size();

  if (csv_profile_enabled(resolved)) {
    fmt::print(
        stderr,
        "load_csv profile [{}]: total={:.2f} ms, detect_sep={:.2f} ms, parse_records={:.2f} ms, "
        "build_dataframe={:.2f} ms, case_filter={:.2f} ms, scan_cells={:.2f} ms, infer_columns={:.2f} ms, "
        "cases(raw/selected)={}/{}, features={}\n",
        resolved.filepath,
        profile_stats.total_ms,
        profile_stats.detect_sep_ms,
        profile_stats.parse_records_ms,
        profile_stats.build_dataframe_ms,
        build_profile.filter_rows_ms,
        build_profile.scan_cells_ms,
        build_profile.infer_columns_ms,
        profile_stats.total_rows,
        profile_stats.selected_rows,
        profile_stats.columns);
  }

  return out;
}

CsvBatch read_csv_batch(std::string_view filepath, const ReadCsvOptions& options) {
  ReadCsvOptions resolved = options;
  resolved.filepath = std::string(filepath);
  return read_csv_batch(resolved);
}

pluss::table::DataFrame load_csv_records(const std::vector<std::vector<std::string>>& records,
                                         const ReadCsvOptions& options) {
  auto batch = read_csv_records_batch(records, options);
  return pluss::table::DataFrame(std::move(batch.features), std::move(batch.cases));
}

pluss::table::DataFrame load_csv(const ReadCsvOptions& options) {
  auto batch = read_csv_batch(options);
  return pluss::table::DataFrame(std::move(batch.features), std::move(batch.cases));
}

pluss::table::DataFrame load_csv(std::string_view filepath, const ReadCsvOptions& options) {
  auto batch = read_csv_batch(filepath, options);
  return pluss::table::DataFrame(std::move(batch.features), std::move(batch.cases));
}

pluss::table::DataFrame load_csv(std::string_view filepath,
                                 char sep,
                                 std::optional<std::size_t> ncases,
                                 std::vector<std::string> usefeats,
                                 std::optional<std::string> case_col,
                                 BadLinePolicy on_bad_lines) {
  ReadCsvOptions options;
  options.sep = sep;
  options.ncases = ncases;
  options.usefeats = std::move(usefeats);
  options.case_col = std::move(case_col);
  options.on_bad_lines = on_bad_lines;
  return load_csv(filepath, options);
}

pluss::table::DataFrame load_csv(std::string_view filepath,
                                 char sep,
                                 std::optional<std::size_t> ncases,
                                 std::initializer_list<std::string_view> usefeats,
                                 std::optional<std::string> case_col,
                                 BadLinePolicy on_bad_lines) {
  std::vector<std::string> feats;
  feats.reserve(usefeats.size());
  for (const auto name : usefeats) {
    feats.emplace_back(name);
  }
  return load_csv(filepath, sep, ncases, std::move(feats), std::move(case_col), on_bad_lines);
}

void save_csv(const pluss::table::DataFrame& frame,
              std::string_view filepath,
              const WriteCsvOptions& options) {
  PLUSS_CHECK(frame.defined(), "save_csv: input frame is undefined");
  PLUSS_CHECK(!filepath.empty(), "save_csv: filepath is empty");

  std::ofstream out(std::string(filepath), std::ios::out | std::ios::trunc | std::ios::binary);
  PLUSS_CHECK(out.is_open(), "save_csv: failed to open output file '" + std::string(filepath) + "'");

  const auto sep = options.sep;
  const auto names = frame.features();
  std::vector<pluss::table::Slate> cols;
  cols.reserve(names.size());
  for (const auto& name : names) {
    cols.push_back(slate_for_csv_io(frame.features(name)));
  }
  const bool include_cases = options.add_cases || options.cases;
  const auto case_values = include_cases ? slate_for_csv_io(frame.labels().values()) : pluss::table::Slate{};

  if (options.header) {
    bool first = true;
    if (include_cases) {
      out << escape_csv_cell(options.case_label.value_or("case"), options);
      first = false;
    }
    for (const auto& name : names) {
      if (!first) {
        out << sep;
      }
      out << escape_csv_cell(name, options);
      first = false;
    }
    out << options.line_terminator;
  }

  for (std::int64_t r = 0; r < frame.ncases(); ++r) {
    bool first = true;
    if (include_cases) {
      out << escape_csv_cell(slate_value_for_csv(case_values, r, options), options);
      first = false;
    }
    for (const auto& col : cols) {
      if (!first) {
        out << sep;
      }
      out << escape_csv_cell(slate_value_for_csv(col, r, options), options);
      first = false;
    }
    out << options.line_terminator;
  }
}

struct CsvCaseReader::Impl {
  detail::ResolvedSource source;
  ReadCsvOptions options;
  std::ifstream input;
  std::size_t batch_size{0};
  std::size_t emitted{0};
  std::vector<std::vector<std::string>> preamble;
  std::vector<std::string> schema_names;
  std::vector<pluss::table::SlateKind> schema_kinds;
  std::vector<std::optional<pluss::codem::ScalarType>> schema_series_types;

  Impl(
      const std::string_view filepath,
      const std::size_t requested_batch_size,
      const ReadCsvOptions& requested_options)
      : source(detail::resolve_local_or_http_source(
            filepath,
            "CsvCaseReader")),
        options(requested_options),
        input(),
        batch_size(requested_batch_size) {
    PLUSS_CHECK(batch_size > 0, "CsvCaseReader: case_batch_size must be positive");
    PLUSS_CHECK(options.skipcases.empty() && options.skipcases_first_n == 0,
                "CsvCaseReader: skipcases policies are not supported by streaming reads");
    PLUSS_CHECK(options.skipfooter == 0,
                "CsvCaseReader: skipfooter is not supported by streaming reads");
    options.filepath = source.local_path();
    options.sep = detect_separator(options);
    options.chunksize.reset();

    input.open(options.filepath, std::ios::in | std::ios::binary);
    PLUSS_CHECK(
        input.is_open(),
        "CsvCaseReader: failed to open file '" + options.filepath + "'");
    if (options.header) {
      preamble.reserve(static_cast<std::size_t>(options.header_row + 1));
      std::vector<std::string> record;
      for (std::int64_t row = 0; row <= options.header_row; ++row) {
        PLUSS_CHECK(
            parse_record(input, options, record),
            "CsvCaseReader: file ended before the requested header row");
        preamble.push_back(record);
      }
    }
  }
};

CsvCaseReader::CsvCaseReader(
    const std::string_view filepath,
    const std::size_t case_batch_size)
    : CsvCaseReader(filepath, case_batch_size, ReadCsvOptions{}) {}

CsvCaseReader::CsvCaseReader(
    const std::string_view filepath,
    const std::size_t case_batch_size,
    const ReadCsvOptions& options)
    : impl_(std::make_unique<Impl>(filepath, case_batch_size, options)) {}

CsvCaseReader::CsvCaseReader(CsvCaseReader&&) noexcept = default;
CsvCaseReader& CsvCaseReader::operator=(CsvCaseReader&&) noexcept = default;
CsvCaseReader::~CsvCaseReader() = default;

bool CsvCaseReader::next(pluss::table::DataFrame& output) {
  PLUSS_CHECK(impl_ != nullptr, "CsvCaseReader::next: reader was moved from");
  if (impl_->options.ncases.has_value() &&
      impl_->emitted >= *impl_->options.ncases) {
    return false;
  }

  auto records = impl_->preamble;
  const auto remaining = impl_->options.ncases.has_value()
      ? *impl_->options.ncases - impl_->emitted
      : impl_->batch_size;
  const auto wanted = std::min(impl_->batch_size, remaining);
  records.reserve(records.size() + wanted);

  std::vector<std::string> record;
  for (std::size_t row = 0; row < wanted; ++row) {
    if (!parse_record(impl_->input, impl_->options, record)) {
      break;
    }
    records.push_back(record);
  }
  const auto parsed = records.size() - impl_->preamble.size();
  if (parsed == 0) {
    return false;
  }

  auto batch_options = impl_->options;
  batch_options.ncases.reset();
  const auto batch = build_batch_from_records(records, batch_options, nullptr);
  output = pluss::table::DataFrame(batch.features, batch.cases);
  if (impl_->schema_names.empty()) {
    impl_->schema_names = output.features();
    impl_->schema_kinds.reserve(static_cast<std::size_t>(output.nfeatures()));
    impl_->schema_series_types.reserve(static_cast<std::size_t>(output.nfeatures()));
    for (const auto& feature : impl_->schema_names) {
      const auto column = output.features(feature);
      impl_->schema_kinds.push_back(column.kind());
      impl_->schema_series_types.push_back(
          column.kind() == pluss::table::SlateKind::Series
              ? std::optional{column.as_series().scalar_type()}
              : std::nullopt);
    }
  } else {
    PLUSS_CHECK(
        output.features() == impl_->schema_names,
        "CsvCaseReader::next: feature schema changed between batches");
    std::vector<pluss::table::Slate> columns;
    columns.reserve(impl_->schema_names.size());
    for (const auto& name : impl_->schema_names) {
      columns.push_back(output.features(name));
    }
    bool converted_any = false;
    for (std::size_t i = 0; i < impl_->schema_names.size(); ++i) {
      const auto& column = columns[i];
      const auto same_kind = column.kind() == impl_->schema_kinds[i];
      const auto same_dtype =
          column.kind() != pluss::table::SlateKind::Series ||
          column.as_series().scalar_type() == *impl_->schema_series_types[i];
      if (same_kind && same_dtype) {
        continue;
      }

      pluss::table::SlateCastTarget target =
          pluss::table::SlateCastTarget::Series;
      switch (impl_->schema_kinds[i]) {
        case pluss::table::SlateKind::Series:
          target = pluss::table::SlateCastTarget::Series;
          break;
        case pluss::table::SlateKind::Note:
          target = pluss::table::SlateCastTarget::Note;
          break;
        case pluss::table::SlateKind::Timeline:
          target = pluss::table::SlateCastTarget::Timeline;
          break;
        case pluss::table::SlateKind::Duration:
          target = pluss::table::SlateCastTarget::Duration;
          break;
      }
      const auto converted = [&]() {
        switch (target) {
          case pluss::table::SlateCastTarget::Series:
            return column.cast({.series_dtype = impl_->schema_series_types[i]}).series();
          case pluss::table::SlateCastTarget::Note:
            return column.cast().note();
          case pluss::table::SlateCastTarget::Timeline:
            return column.cast().timeline();
          case pluss::table::SlateCastTarget::Duration:
            return column.cast().duration();
        }
        PLUSS_FAIL("CsvCaseReader::next: unsupported schema kind");
      }();
      auto lost_observation =
          column.notnull().as_series().logical_and(
              converted.notnull().as_series().logical_not());
      if (column.kind() == pluss::table::SlateKind::Series &&
          converted.kind() == pluss::table::SlateKind::Series) {
        const auto round_trip =
            converted.as_series().to(column.as_series().scalar_type());
        lost_observation = lost_observation.logical_or(
            column.notnull().as_series().logical_and(
                round_trip.eq(column.as_series()).logical_not()));
      }
      PLUSS_CHECK(
          !lost_observation.any().item<bool>(),
          "CsvCaseReader::next: inferred schema changed for feature '" +
              impl_->schema_names[i] +
              "' and conversion would discard an observed value; supply "
              "ReadCsvOptions::dtype so every streamed batch has one explicit "
              "schema");
      columns[i] = converted;
      converted_any = true;
    }
    if (converted_any) {
      auto cases = output.labels().values();
      std::vector<std::pair<std::string, pluss::table::Slate>> typed_columns;
      typed_columns.reserve(columns.size());
      for (std::size_t i = 0; i < columns.size(); ++i) {
        typed_columns.emplace_back(impl_->schema_names[i], std::move(columns[i]));
      }
      output = pluss::table::DataFrame(
          std::move(typed_columns), std::move(cases));
    }
  }
  impl_->emitted += parsed;
  return true;
}

std::size_t CsvCaseReader::cases_read() const noexcept {
  return impl_ != nullptr ? impl_->emitted : 0;
}

} // namespace pluss::loader
