#include "loader/CsvLoader.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include "CodeM/core/Errors.h"
#include "CodeM/core/ScalarType.h"
#include "table/DataFrame.h"
#include "table/Thread.h"

namespace pluss::loader {

namespace {

enum class InferredKind : std::uint8_t {
  Bool,
  Int64,
  Float64,
  DateTime,
  Text,
};

using Thread = pluss::table::Thread;

std::string lower_copy(std::string s) {
  for (auto& ch : s) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return s;
}

std::string trim_ascii(std::string_view s) {
  std::size_t b = 0;
  std::size_t e = s.size();
  while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) {
    ++b;
  }
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) {
    --e;
  }
  return std::string(s.substr(b, e - b));
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

char effective_separator(const ReadCsvOptions& options) {
  if (options.delimiter.has_value()) {
    return *options.delimiter;
  }
  return options.sep;
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

bool is_na_token(std::string_view token, const ReadCsvOptions& options) {
  if (!options.na_filter) {
    return false;
  }
  const auto t = trim_ascii(token);
  if (t.empty()) {
    return true;
  }
  const auto tl = lower_copy(t);

  if (options.na_values.find(t) != options.na_values.end() ||
      options.na_values.find(tl) != options.na_values.end()) {
    return true;
  }

  if (!options.keep_default_na) {
    return false;
  }

  static const std::unordered_set<std::string> kDefaultNa{
      "", "na", "n/a", "nan", "null", "none", "nat", "#n/a", "-nan", "-1.#ind", "1.#qnan"};
  return kDefaultNa.find(tl) != kDefaultNa.end();
}

bool parse_bool_token(std::string_view token, const ReadCsvOptions& options, bool& out) {
  const auto t = trim_ascii(token);
  if (t.empty()) {
    return false;
  }
  if (options.true_values.find(t) != options.true_values.end() ||
      options.true_values.find(lower_copy(t)) != options.true_values.end()) {
    out = true;
    return true;
  }
  if (options.false_values.find(t) != options.false_values.end() ||
      options.false_values.find(lower_copy(t)) != options.false_values.end()) {
    out = false;
    return true;
  }
  return false;
}

std::string normalize_numeric(std::string_view token, const ReadCsvOptions& options) {
  std::string s = trim_ascii(token);
  if (options.thousands.has_value()) {
    s.erase(std::remove(s.begin(), s.end(), *options.thousands), s.end());
  }
  if (options.decimal != '.') {
    std::replace(s.begin(), s.end(), options.decimal, '.');
  }
  return s;
}

bool parse_int64_token(std::string_view token, const ReadCsvOptions& options, std::int64_t& out) {
  auto s = normalize_numeric(token, options);
  if (s.empty()) {
    return false;
  }
  if (s.find('.') != std::string::npos || s.find('e') != std::string::npos || s.find('E') != std::string::npos) {
    return false;
  }
  const auto* b = s.data();
  const auto* e = s.data() + s.size();
  auto res = std::from_chars(b, e, out);
  return res.ec == std::errc{} && res.ptr == e;
}

bool parse_double_token(std::string_view token, const ReadCsvOptions& options, double& out) {
  auto s = normalize_numeric(token, options);
  if (s.empty()) {
    return false;
  }
  char* end = nullptr;
  out = std::strtod(s.c_str(), &end);
  return end != nullptr && *end == '\0' && std::isfinite(out);
}

std::optional<pluss::Series> validity_mask_from_valid(const std::vector<bool>& valid) {
  const auto n = static_cast<std::int64_t>(valid.size());
  const auto n_valid = static_cast<std::int64_t>(std::count(valid.begin(), valid.end(), true));
  if (n_valid == n) {
    return std::nullopt;
  }
  auto mask = pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
  auto* p = reinterpret_cast<bool*>(mask.raw_data());
  for (std::int64_t i = 0; i < n; ++i) {
    p[i] = valid[static_cast<std::size_t>(i)];
  }
  return mask;
}

Thread make_series_thread_bool(const std::vector<bool>& values, const std::vector<bool>& valid) {
  const auto n = static_cast<std::int64_t>(values.size());
  auto s = pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
  auto* p = reinterpret_cast<bool*>(s.raw_data());
  for (std::int64_t i = 0; i < n; ++i) {
    p[i] = values[static_cast<std::size_t>(i)];
  }
  return Thread(s, validity_mask_from_valid(valid));
}

Thread make_series_thread_i64(const std::vector<std::int64_t>& values, const std::vector<bool>& valid) {
  const auto n = static_cast<std::int64_t>(values.size());
  auto s = pluss::Series::empty({n}, pluss::codem::ScalarType::Int64);
  if (!values.empty()) {
    std::memcpy(s.raw_data(), values.data(), values.size() * sizeof(std::int64_t));
  }
  return Thread(s, validity_mask_from_valid(valid));
}

Thread make_series_thread_f64(const std::vector<double>& values, const std::vector<bool>& valid) {
  const auto n = static_cast<std::int64_t>(values.size());
  auto s = pluss::Series::empty({n}, pluss::codem::ScalarType::Float64);
  if (!values.empty()) {
    std::memcpy(s.raw_data(), values.data(), values.size() * sizeof(double));
  }
  return Thread(s, validity_mask_from_valid(valid));
}

Thread make_note_thread(const std::vector<std::string>& values, const std::vector<bool>& valid) {
  std::vector<std::string_view> views;
  views.reserve(values.size());
  for (const auto& v : values) {
    views.emplace_back(v);
  }
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(views.size())};
  auto note = pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(views.data(), views.size()),
      std::span<const std::int64_t>(shape.data(), shape.size()));
  return Thread(note, validity_mask_from_valid(valid));
}

bool try_make_timeline_thread(const std::vector<std::string>& values,
                              const std::vector<bool>& valid,
                              Thread& out_thread) {
  std::vector<std::string> filled = values;
  for (std::size_t i = 0; i < filled.size(); ++i) {
    if (!valid[i]) {
      filled[i] = "1970-01-01T00:00:00Z";
    }
  }

  std::vector<std::string_view> views;
  views.reserve(filled.size());
  for (const auto& v : filled) {
    views.emplace_back(v);
  }

  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(views.size())};
  try {
    auto timeline = pluss::codeh::ops_internal::timeline_from_utf8(
        std::span<const std::string_view>(views.data(), views.size()),
        std::span<const std::int64_t>(shape.data(), shape.size()));
    out_thread = Thread(timeline, validity_mask_from_valid(valid));
    return true;
  } catch (...) {
    return false;
  }
}

bool should_parse_as_datetime(const std::string& name,
                              std::size_t index,
                              const ReadCsvOptions& options,
                              const std::unordered_map<std::string, std::string>& dtype_lower) {
  for (const auto& n : options.parse_dates) {
    if (n == name) {
      return true;
    }
  }
  for (const auto idx : options.parse_dates_idx) {
    if (idx == index) {
      return true;
    }
  }
  const auto it = dtype_lower.find(name);
  if (it != dtype_lower.end()) {
    const auto& dt = it->second;
    return dt == "datetime" || dt == "datetime64" || dt == "timeline" || dt == "date";
  }
  return false;
}

InferredKind forced_kind_for(const std::string& name,
                             const std::unordered_map<std::string, std::string>& dtype_lower,
                             std::optional<InferredKind>& forced) {
  forced.reset();
  const auto it = dtype_lower.find(name);
  if (it == dtype_lower.end()) {
    return InferredKind::Text;
  }

  const auto& d = it->second;
  if (d == "bool" || d == "boolean") {
    forced = InferredKind::Bool;
  } else if (d == "int" || d == "int64" || d == "long") {
    forced = InferredKind::Int64;
  } else if (d == "float" || d == "float64" || d == "double") {
    forced = InferredKind::Float64;
  } else if (d == "datetime" || d == "datetime64" || d == "timeline" || d == "date") {
    forced = InferredKind::DateTime;
  } else if (d == "str" || d == "string" || d == "object" || d == "text" || d == "note") {
    forced = InferredKind::Text;
  }
  return forced.value_or(InferredKind::Text);
}

Thread infer_column_thread(const std::string& col_name,
                           std::size_t source_col_index,
                           const std::vector<std::string>& raw_values,
                           const ReadCsvOptions& options,
                           const std::unordered_map<std::string, std::string>& dtype_lower) {
  const auto n = raw_values.size();
  std::vector<bool> valid(n, true);
  std::vector<std::string> values = raw_values;

  for (std::size_t i = 0; i < n; ++i) {
    if (is_na_token(values[i], options)) {
      valid[i] = false;
      values[i].clear();
    }
  }

  std::optional<InferredKind> forced;
  (void)forced_kind_for(col_name, dtype_lower, forced);

  const bool parse_dt = should_parse_as_datetime(col_name, source_col_index, options, dtype_lower);
  if (parse_dt || (forced.has_value() && *forced == InferredKind::DateTime)) {
    Thread out;
    if (try_make_timeline_thread(values, valid, out)) {
      return out;
    }
    if (forced.has_value() && *forced == InferredKind::DateTime) {
      PLUSS_FAIL("load_csv: failed to parse datetime column '" + col_name + "'");
    }
  }

  {
    const bool try_bool = !forced.has_value() || *forced == InferredKind::Bool;
    if (try_bool) {
      std::vector<bool> parsed(n, false);
      bool ok = true;
      for (std::size_t i = 0; i < n; ++i) {
        if (!valid[i]) continue;
        bool v = false;
        if (!parse_bool_token(values[i], options, v)) {
          ok = false;
          break;
        }
        parsed[i] = v;
      }
      if (ok) {
        return make_series_thread_bool(parsed, valid);
      }
      if (forced.has_value() && *forced == InferredKind::Bool) {
        PLUSS_FAIL("load_csv: failed to parse bool column '" + col_name + "'");
      }
    }
  }

  {
    const bool try_i64 = !forced.has_value() || *forced == InferredKind::Int64;
    if (try_i64) {
      std::vector<std::int64_t> parsed(n, 0);
      bool ok = true;
      for (std::size_t i = 0; i < n; ++i) {
        if (!valid[i]) continue;
        std::int64_t v = 0;
        if (!parse_int64_token(values[i], options, v)) {
          ok = false;
          break;
        }
        parsed[i] = v;
      }
      if (ok) {
        return make_series_thread_i64(parsed, valid);
      }
      if (forced.has_value() && *forced == InferredKind::Int64) {
        PLUSS_FAIL("load_csv: failed to parse int64 column '" + col_name + "'");
      }
    }
  }

  {
    const bool try_f64 = !forced.has_value() || *forced == InferredKind::Float64;
    if (try_f64) {
      std::vector<double> parsed(n, 0.0);
      bool ok = true;
      for (std::size_t i = 0; i < n; ++i) {
        if (!valid[i]) continue;
        double v = 0.0;
        if (!parse_double_token(values[i], options, v)) {
          ok = false;
          break;
        }
        parsed[i] = v;
      }
      if (ok) {
        return make_series_thread_f64(parsed, valid);
      }
      if (forced.has_value() && *forced == InferredKind::Float64) {
        PLUSS_FAIL("load_csv: failed to parse float64 column '" + col_name + "'");
      }
    }
  }

  {
    Thread out;
    if (try_make_timeline_thread(values, valid, out)) {
      return out;
    }
  }

  return make_note_thread(values, valid);
}

std::vector<std::size_t> resolve_usecols(const std::vector<std::string>& names,
                                         const ReadCsvOptions& options) {
  std::vector<std::size_t> out;

  if (options.usecols.empty() && options.usecols_idx.empty()) {
    out.resize(names.size());
    for (std::size_t i = 0; i < names.size(); ++i) {
      out[i] = i;
    }
    return out;
  }

  for (const auto idx : options.usecols_idx) {
    PLUSS_CHECK(idx < names.size(), fmt::format("load_csv: usecols_idx {} out of range", idx));
    out.push_back(idx);
  }

  for (const auto& name : options.usecols) {
    const auto it = std::find(names.begin(), names.end(), name);
    PLUSS_CHECK(it != names.end(), "load_csv: unknown usecols name '" + name + "'");
    out.push_back(static_cast<std::size_t>(std::distance(names.begin(), it)));
  }

  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return out;
}

} // namespace

pluss::table::DataFrame load_csv(const ReadCsvOptions& options) {
  PLUSS_CHECK(!options.filepath.empty(), "load_csv: filepath is empty");

  std::ifstream in(options.filepath, std::ios::in | std::ios::binary);
  PLUSS_CHECK(in.is_open(), "load_csv: failed to open file '" + options.filepath + "'");

  std::vector<std::vector<std::string>> records;
  records.reserve(4096);

  std::vector<std::string> row;
  std::size_t logical_row = 0;
  while (parse_record(in, options, row)) {
    if (options.skiprows_first_n > 0 && logical_row < options.skiprows_first_n) {
      ++logical_row;
      continue;
    }
    if (options.skiprows.find(logical_row) != options.skiprows.end()) {
      ++logical_row;
      continue;
    }

    if (options.skipinitialspace) {
      for (auto& f : row) {
        ltrim_inplace(f);
      }
    }

    records.push_back(row);
    ++logical_row;
  }

  PLUSS_CHECK(!records.empty(), "load_csv: no rows parsed from csv");

  std::vector<std::string> names;
  std::size_t data_start = 0;

  if (options.header) {
    PLUSS_CHECK(options.header_row >= 0, "load_csv: header_row must be >= 0");
    const auto h = static_cast<std::size_t>(options.header_row);
    PLUSS_CHECK(h < records.size(), "load_csv: header_row is out of range");
    names = records[h];
    data_start = h + 1;
  } else {
    const auto ncol = records.front().size();
    names.reserve(ncol);
    for (std::size_t i = 0; i < ncol; ++i) {
      names.push_back(fmt::format("col_{}", i));
    }
    data_start = 0;
  }

  if (!options.names.empty()) {
    names = options.names;
  }

  PLUSS_CHECK(!names.empty(), "load_csv: inferred zero columns");

  if (options.skipfooter > 0) {
    const auto n_data = records.size() > data_start ? (records.size() - data_start) : 0;
    PLUSS_CHECK(options.skipfooter <= n_data, "load_csv: skipfooter exceeds number of data rows");
  }

  const auto usecols = resolve_usecols(names, options);

  std::optional<std::size_t> index_col;
  if (options.index_col_idx.has_value()) {
    PLUSS_CHECK(*options.index_col_idx < names.size(), "load_csv: index_col_idx is out of range");
    index_col = *options.index_col_idx;
  }
  if (options.index_col.has_value()) {
    const auto it = std::find(names.begin(), names.end(), *options.index_col);
    PLUSS_CHECK(it != names.end(), "load_csv: unknown index_col name '" + *options.index_col + "'");
    index_col = static_cast<std::size_t>(std::distance(names.begin(), it));
  }

  std::vector<std::size_t> selected_cols;
  selected_cols.reserve(usecols.size());
  for (const auto c : usecols) {
    if (index_col.has_value() && c == *index_col) {
      continue;
    }
    selected_cols.push_back(c);
  }

  std::vector<std::string> selected_names;
  selected_names.reserve(selected_cols.size());
  for (const auto c : selected_cols) {
    selected_names.push_back(names[c]);
  }

  std::vector<std::vector<std::string>> values_by_col(selected_cols.size());
  std::vector<std::string> index_values;

  std::size_t bad_lines = 0;
  std::size_t accepted_rows = 0;

  const std::size_t tail_cut = options.skipfooter;
  const std::size_t final_end = records.size() - tail_cut;

  for (std::size_t r = data_start; r < final_end; ++r) {
    const auto& rec = records[r];

    if (rec.size() != names.size()) {
      ++bad_lines;
      if (options.on_bad_lines == BadLinePolicy::Error) {
        PLUSS_FAIL(fmt::format("load_csv: bad line at data row {} (expected {} fields, got {})",
                               r - data_start,
                               names.size(),
                               rec.size()));
      }
      if (options.on_bad_lines == BadLinePolicy::Warn) {
        ::pluss::codem::warn(fmt::format("load_csv: skipping malformed row {}", r - data_start));
      }
      continue;
    }

    if (options.nrows.has_value() && accepted_rows >= *options.nrows) {
      break;
    }

    for (std::size_t i = 0; i < selected_cols.size(); ++i) {
      values_by_col[i].push_back(rec[selected_cols[i]]);
    }

    if (index_col.has_value()) {
      index_values.push_back(rec[*index_col]);
    }

    ++accepted_rows;
  }

  if (options.on_bad_lines == BadLinePolicy::Warn && bad_lines > 0) {
    ::pluss::codem::warn(fmt::format("load_csv: skipped {} malformed rows", bad_lines));
  }

  std::unordered_map<std::string, std::string> dtype_lower;
  dtype_lower.reserve(options.dtype.size());
  for (const auto& kv : options.dtype) {
    dtype_lower.emplace(kv.first, lower_copy(kv.second));
  }

  std::vector<std::pair<std::string, pluss::table::Thread>> cols;
  cols.reserve(selected_cols.size());
  for (std::size_t i = 0; i < selected_cols.size(); ++i) {
    cols.emplace_back(
        selected_names[i],
        infer_column_thread(selected_names[i], selected_cols[i], values_by_col[i], options, dtype_lower));
  }

  std::optional<pluss::table::Thread> case_index = std::nullopt;
  if (index_col.has_value()) {
    const auto idx_name = names[*index_col];
    case_index = infer_column_thread(idx_name, *index_col, index_values, options, dtype_lower);
  }

  return pluss::table::DataFrame(std::move(cols), std::move(case_index));
}

} // namespace pluss::loader
