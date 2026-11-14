#include "loader/ExcelLoader.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>


#include "CodeM/core/Complex.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/ScalarType.h"
#include "loader/CsvLoader.h"
#include "loader/SourceResolver.h"
#include "table/DataFrame.h"
#include "table/Slate.h"
#include <pluss/tils/kit/output.h>

#if defined(PLUSSMIC_HAS_LIBZIP) && PLUSSMIC_HAS_LIBZIP
#include <zip.h>
#endif

namespace pluss::loader {

namespace {

std::string lower_copy(std::string s) {
  for (auto& ch : s) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return s;
}

std::string normalize_sheet_name(std::string name) {
  name = lower_copy(std::move(name));
  name.erase(std::remove_if(name.begin(),
                            name.end(),
                            [](char c) { return std::isspace(static_cast<unsigned char>(c)) != 0 || c == '_'; }),
             name.end());
  return name;
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
    const auto valid = pluss::table::detail::validity_mask(t)->host<bool>();
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
  const auto values = s.host<std::int64_t>();
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

pluss::table::Slate slate_for_excel_io(const pluss::table::Slate& t) {
  if (t.kind() == pluss::table::SlateKind::Timeline) {
    return temporal_slate_for_io(t);
  }
  return t;
}

std::string xml_escape(std::string_view input) {
  std::string out;
  out.reserve(input.size() + 16);
  for (const char ch : input) {
    switch (ch) {
      case '&':
        out += "&amp;";
        break;
      case '<':
        out += "&lt;";
        break;
      case '>':
        out += "&gt;";
        break;
      case '"':
        out += "&quot;";
        break;
      case '\'':
        out += "&apos;";
        break;
      default:
        out.push_back(ch);
        break;
    }
  }
  return out;
}

std::string scalar_for_excel(const pluss::Series& s, std::int64_t row) {
  const auto st = s.scalar_type();
  const auto* base = s.raw_data();
  switch (st) {
    case pluss::codem::ScalarType::Bool:
      return reinterpret_cast<const bool*>(base)[row] ? "true" : "false";
    case pluss::codem::ScalarType::Int8:
      return pluss::tils::OutputEngine::format("{}", static_cast<int>(reinterpret_cast<const std::int8_t*>(base)[row]));
    case pluss::codem::ScalarType::Int16:
      return pluss::tils::OutputEngine::format("{}", reinterpret_cast<const std::int16_t*>(base)[row]);
    case pluss::codem::ScalarType::Int32:
      return pluss::tils::OutputEngine::format("{}", reinterpret_cast<const std::int32_t*>(base)[row]);
    case pluss::codem::ScalarType::Int64:
      return pluss::tils::OutputEngine::format("{}", reinterpret_cast<const std::int64_t*>(base)[row]);
    case pluss::codem::ScalarType::UInt8:
      return pluss::tils::OutputEngine::format("{}", static_cast<unsigned>(reinterpret_cast<const std::uint8_t*>(base)[row]));
    case pluss::codem::ScalarType::UInt16:
      return pluss::tils::OutputEngine::format("{}", reinterpret_cast<const std::uint16_t*>(base)[row]);
    case pluss::codem::ScalarType::UInt32:
      return pluss::tils::OutputEngine::format("{}", reinterpret_cast<const std::uint32_t*>(base)[row]);
    case pluss::codem::ScalarType::UInt64:
      return pluss::tils::OutputEngine::format("{}", reinterpret_cast<const std::uint64_t*>(base)[row]);
    case pluss::codem::ScalarType::Float16:
      return pluss::tils::OutputEngine::format("{:.6g}",
                         pluss::codem::float16_to_float(reinterpret_cast<const pluss::codem::Float16*>(base)[row]));
    case pluss::codem::ScalarType::BFloat16:
      return pluss::tils::OutputEngine::format("{:.6g}",
                         pluss::codem::bfloat16_to_float(reinterpret_cast<const pluss::codem::BFloat16*>(base)[row]));
    case pluss::codem::ScalarType::Float32:
      return pluss::tils::OutputEngine::format("{:.8g}", reinterpret_cast<const float*>(base)[row]);
    case pluss::codem::ScalarType::Float64:
      return pluss::tils::OutputEngine::format("{:.12g}", reinterpret_cast<const double*>(base)[row]);
    case pluss::codem::ScalarType::Complex64: {
      const auto v = reinterpret_cast<const pluss::codem::Complex64*>(base)[row];
      return pluss::tils::OutputEngine::format("({:.6g}{:+.6g}j)", v.real(), v.imag());
    }
    case pluss::codem::ScalarType::Complex128: {
      const auto v = reinterpret_cast<const pluss::codem::Complex128*>(base)[row];
      return pluss::tils::OutputEngine::format("({:.6g}{:+.6g}j)", v.real(), v.imag());
    }
    default:
      return "";
  }
}

std::string slate_value_for_excel(const pluss::table::Slate& t, std::int64_t row, std::string_view null_rep) {
  if (slate_value_is_missing_for_io(t, row)) {
    return std::string(null_rep);
  }

  switch (t.kind()) {
    case pluss::table::SlateKind::Series:
      return scalar_for_excel(t.as_series(), row);
    case pluss::table::SlateKind::Note:
      return std::string(t.as_note().at(row));
    case pluss::table::SlateKind::Timeline:
      return pluss::tils::OutputEngine::format("{}", t.as_timeline().ticks()[static_cast<std::size_t>(row)]);
    case pluss::table::SlateKind::Duration:
      return pluss::tils::OutputEngine::format("{}", t.as_duration().host()[static_cast<std::size_t>(row)]);
  }
  PLUSS_FAIL("save_excel: unsupported Slate kind");
}

std::string column_letters(std::int64_t zero_based_col) {
  std::string out;
  std::int64_t col = zero_based_col + 1;
  while (col > 0) {
    const auto rem = static_cast<int>((col - 1) % 26);
    out.push_back(static_cast<char>('A' + rem));
    col = (col - 1) / 26;
  }
  std::reverse(out.begin(), out.end());
  return out;
}

using CellTable = std::vector<std::vector<std::string>>;

std::string trim_ascii(std::string_view s) {
  std::size_t b = 0;
  std::size_t e = s.size();
  while (b < e && std::isspace(static_cast<unsigned char>(s[b])) != 0) ++b;
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])) != 0) --e;
  return std::string(s.substr(b, e - b));
}

bool has_non_empty_cell(const std::vector<std::string>& row) {
  for (const auto& cell : row) {
    if (!trim_ascii(cell).empty()) {
      return true;
    }
  }
  return false;
}

bool has_non_empty_in_col(const CellTable& table, std::size_t col) {
  for (const auto& row : table) {
    if (col < row.size() && !trim_ascii(row[col]).empty()) {
      return true;
    }
  }
  return false;
}

void make_rectangular(CellTable& table) {
  std::size_t max_cols = 0;
  for (const auto& row : table) {
    max_cols = std::max(max_cols, row.size());
  }
  for (auto& row : table) {
    row.resize(max_cols);
  }
}

void trim_outer_empty(CellTable& table) {
  if (table.empty()) return;
  make_rectangular(table);

  std::size_t top = 0;
  while (top < table.size() && !has_non_empty_cell(table[top])) ++top;
  if (top == table.size()) {
    table.clear();
    return;
  }

  std::size_t bottom = table.size() - 1;
  while (bottom > top && !has_non_empty_cell(table[bottom])) --bottom;

  const std::size_t ncols = table.front().size();
  std::size_t left = 0;
  while (left < ncols && !has_non_empty_in_col(table, left)) ++left;
  if (left == ncols) {
    table.clear();
    return;
  }

  std::size_t right = ncols - 1;
  while (right > left && !has_non_empty_in_col(table, right)) --right;

  CellTable out;
  out.reserve(bottom - top + 1);
  for (std::size_t r = top; r <= bottom; ++r) {
    out.emplace_back(table[r].begin() + static_cast<std::ptrdiff_t>(left),
                     table[r].begin() + static_cast<std::ptrdiff_t>(right + 1));
  }
  table = std::move(out);
}

CellTable crop_table(const CellTable& table,
                     std::size_t row0,
                     std::size_t row1,
                     std::size_t col0,
                     std::size_t col1) {
  CellTable out;
  if (table.empty()) return out;
  out.reserve(row1 - row0 + 1);
  for (std::size_t r = row0; r <= row1; ++r) {
    std::vector<std::string> row;
    row.reserve(col1 - col0 + 1);
    for (std::size_t c = col0; c <= col1; ++c) {
      row.push_back(table[r][c]);
    }
    out.push_back(std::move(row));
  }
  return out;
}

std::optional<std::array<std::size_t, 4>> detect_auto_table_bounds(const CellTable& table,
                                                                    const ReadExcelOptions& options) {
  if (table.empty()) return std::nullopt;

  const std::size_t rows = table.size();
  const std::size_t cols = table.front().size();
  if (rows == 0 || cols == 0) return std::nullopt;

  std::vector<std::size_t> row_non_empty(rows, 0);
  for (std::size_t r = 0; r < rows; ++r) {
    for (std::size_t c = 0; c < cols; ++c) {
      if (!trim_ascii(table[r][c]).empty()) {
        ++row_non_empty[r];
      }
    }
  }

  const std::size_t min_cells = std::max<std::size_t>(1, options.auto_min_non_empty_cells);
  const std::size_t max_gap = options.auto_max_blank_gap;

  bool active = false;
  std::size_t run_start = 0;
  std::size_t run_last_good = 0;
  std::size_t run_gap = 0;
  std::size_t run_score = 0;

  std::size_t best_start = 0;
  std::size_t best_end = 0;
  std::size_t best_score = 0;
  bool best_valid = false;

  auto flush_run = [&]() {
    if (!active) return;
    const std::size_t run_end = run_last_good;
    const std::size_t run_len = run_end >= run_start ? (run_end - run_start + 1) : 0;
    const std::size_t best_len = best_end >= best_start ? (best_end - best_start + 1) : 0;
    if (!best_valid || run_score > best_score || (run_score == best_score && run_len > best_len)) {
      best_valid = true;
      best_start = run_start;
      best_end = run_end;
      best_score = run_score;
    }
    active = false;
    run_start = 0;
    run_last_good = 0;
    run_gap = 0;
    run_score = 0;
  };

  for (std::size_t r = 0; r < rows; ++r) {
    const bool good = row_non_empty[r] >= min_cells;
    if (good) {
      if (!active) {
        active = true;
        run_start = r;
        run_last_good = r;
        run_gap = 0;
        run_score = row_non_empty[r];
      } else {
        run_last_good = r;
        run_gap = 0;
        run_score += row_non_empty[r];
      }
      continue;
    }
    if (!active) continue;
    if (run_gap < max_gap) {
      ++run_gap;
      continue;
    }
    flush_run();
  }
  flush_run();

  if (!best_valid) return std::nullopt;

  std::vector<std::size_t> col_non_empty(cols, 0);
  for (std::size_t r = best_start; r <= best_end; ++r) {
    for (std::size_t c = 0; c < cols; ++c) {
      if (!trim_ascii(table[r][c]).empty()) {
        ++col_non_empty[c];
      }
    }
  }

  const auto run_rows = best_end - best_start + 1;
  const std::size_t min_col_non_empty = run_rows >= 2 ? 2 : 1;

  std::size_t left = cols;
  std::size_t right = 0;
  for (std::size_t c = 0; c < cols; ++c) {
    if (col_non_empty[c] >= min_col_non_empty) {
      left = std::min(left, c);
      right = std::max(right, c);
    }
  }

  if (left == cols) {
    for (std::size_t c = 0; c < cols; ++c) {
      if (col_non_empty[c] > 0) {
        left = std::min(left, c);
        right = std::max(right, c);
      }
    }
  }

  if (left == cols || right < left) return std::nullopt;
  return std::array<std::size_t, 4>{best_start, best_end, left, right};
}

CellTable apply_excel_table_policies(CellTable table, const ReadExcelOptions& options) {
  if (table.empty()) return table;
  make_rectangular(table);

  auto clamp = [](std::int64_t v, std::size_t max_inclusive) -> std::size_t {
    if (v < 0) return 0;
    const auto uv = static_cast<std::size_t>(v);
    return std::min(uv, max_inclusive);
  };

  if (options.start_row.has_value() || options.end_row.has_value() || options.start_col.has_value() ||
      options.end_col.has_value()) {
    const std::size_t max_row = table.size() - 1;
    const std::size_t max_col = table.front().size() - 1;
    const std::size_t row0 = options.start_row.has_value() ? clamp(*options.start_row, max_row) : 0;
    const std::size_t row1 = options.end_row.has_value() ? clamp(*options.end_row, max_row) : max_row;
    const std::size_t col0 = options.start_col.has_value() ? clamp(*options.start_col, max_col) : 0;
    const std::size_t col1 = options.end_col.has_value() ? clamp(*options.end_col, max_col) : max_col;
    if (row0 <= row1 && col0 <= col1) {
      table = crop_table(table, row0, row1, col0, col1);
    } else {
      table.clear();
      return table;
    }
  }

  if (options.trim_outer_empty) {
    trim_outer_empty(table);
  }
  if (table.empty()) return table;

  if (options.auto_table_block) {
    if (const auto bounds = detect_auto_table_bounds(table, options); bounds.has_value()) {
      table = crop_table(table, (*bounds)[0], (*bounds)[1], (*bounds)[2], (*bounds)[3]);
      if (options.trim_outer_empty) {
        trim_outer_empty(table);
      }
    }
  }

  return table;
}

std::vector<std::string> flattened_header_names(const CellTable& table,
                                                std::size_t header_row,
                                                std::size_t header_rows,
                                                bool header_ffill,
                                                std::string_view joiner) {
  PLUSS_CHECK(!table.empty(), "load_excel: cannot infer headers from empty table");
  const std::size_t nrows = table.size();
  const std::size_t ncols = table.front().size();
  PLUSS_CHECK(header_rows > 0, "load_excel: header_rows must be > 0");
  PLUSS_CHECK(header_row < nrows, "load_excel: header_row out of range");
  PLUSS_CHECK(header_row + header_rows <= nrows, "load_excel: header_row + header_rows exceeds row count");

  std::vector<std::vector<std::string>> tokens(header_rows, std::vector<std::string>(ncols));

  for (std::size_t r = 0; r < header_rows; ++r) {
    std::string carry;
    for (std::size_t c = 0; c < ncols; ++c) {
      auto token = trim_ascii(table[header_row + r][c]);
      if (header_ffill && token.empty() && !carry.empty()) {
        token = carry;
      }
      if (!token.empty()) {
        carry = token;
      }
      tokens[r][c] = std::move(token);
    }
  }

  if (header_ffill) {
    for (std::size_t c = 0; c < ncols; ++c) {
      std::string carry;
      for (std::size_t r = 0; r < header_rows; ++r) {
        if (tokens[r][c].empty() && !carry.empty()) {
          tokens[r][c] = carry;
        } else if (!tokens[r][c].empty()) {
          carry = tokens[r][c];
        }
      }
    }
  }

  std::vector<std::string> names(ncols);
  for (std::size_t c = 0; c < ncols; ++c) {
    std::string name;
    for (std::size_t r = 0; r < header_rows; ++r) {
      const auto& token = tokens[r][c];
      if (token.empty()) continue;
      if (name.empty()) {
        name = token;
        continue;
      }
      // Drop adjacent duplicates introduced by merged-header fill.
      if (name.size() >= token.size() && name.compare(name.size() - token.size(), token.size(), token) == 0) {
        continue;
      }
      name += std::string(joiner);
      name += token;
    }
    if (name.empty()) {
      name = pluss::tils::OutputEngine::format("feat_{}", c);
    }
    names[c] = std::move(name);
  }

  // Enforce uniqueness for downstream frame constructors.
  std::unordered_map<std::string, std::size_t> seen;
  seen.reserve(names.size());
  for (std::size_t c = 0; c < names.size(); ++c) {
    auto base = trim_ascii(names[c]);
    if (base.empty()) {
      base = pluss::tils::OutputEngine::format("feat_{}", c);
    }
    auto [it, inserted] = seen.emplace(base, 1);
    if (inserted) {
      names[c] = std::move(base);
      continue;
    }
    const auto nth = ++(it->second);
    names[c] = pluss::tils::OutputEngine::format("{}_{}", base, nth);
  }

  return names;
}

CellTable apply_excel_header_policies(CellTable table, const ReadExcelOptions& options) {
  if (table.empty() || !options.header) {
    return table;
  }

  make_rectangular(table);

  PLUSS_CHECK(options.header_row >= 0, "load_excel: header_row must be >= 0");
  const std::size_t header_row = static_cast<std::size_t>(options.header_row);
  const std::size_t header_rows = std::max<std::size_t>(1, options.header_rows);
  if (header_row >= table.size()) {
    PLUSS_FAIL("load_excel: header_row out of range after table policy application");
  }
  if (header_row + header_rows > table.size()) {
    PLUSS_FAIL("load_excel: header_row + header_rows out of range after table policy application");
  }

  if (header_rows == 1) {
    return table;
  }

  auto names = flattened_header_names(table, header_row, header_rows, options.header_ffill, options.header_joiner);
  table[header_row] = std::move(names);
  const auto erase_begin = table.begin() + static_cast<std::ptrdiff_t>(header_row + 1);
  const auto erase_end = table.begin() + static_cast<std::ptrdiff_t>(header_row + header_rows);
  table.erase(erase_begin, erase_end);
  return table;
}

#if defined(PLUSSMIC_HAS_LIBZIP) && PLUSSMIC_HAS_LIBZIP

std::string read_zip_entry(zip_t* archive, std::string_view path) {
  zip_stat_t st{};
  zip_stat_init(&st);
  if (zip_stat(archive, std::string(path).c_str(), 0, &st) != 0) {
    return {};
  }
  zip_file_t* file = zip_fopen(archive, std::string(path).c_str(), 0);
  if (file == nullptr) {
    return {};
  }
  std::string out(static_cast<std::size_t>(st.size), '\0');
  const auto read_n = zip_fread(file, out.data(), st.size);
  zip_fclose(file);
  if (read_n < 0 || static_cast<zip_uint64_t>(read_n) != st.size) {
    PLUSS_FAIL("load_excel: failed to read zip entry '" + std::string(path) + "'");
  }
  return out;
}

std::optional<std::string> xml_attr(std::string_view tag, std::string_view attr_name) {
  const std::string pattern = pluss::tils::OutputEngine::format("{}=\"", attr_name);
  const auto p = tag.find(pattern);
  if (p == std::string_view::npos) {
    return std::nullopt;
  }
  const auto v0 = p + pattern.size();
  const auto v1 = tag.find('"', v0);
  if (v1 == std::string_view::npos) {
    return std::nullopt;
  }
  return std::string(tag.substr(v0, v1 - v0));
}

std::string xml_entity_decode(std::string s) {
  auto replace_all = [](std::string& in, std::string_view from, std::string_view to) {
    std::size_t pos = 0;
    while ((pos = in.find(from, pos)) != std::string::npos) {
      in.replace(pos, from.size(), to);
      pos += to.size();
    }
  };
  replace_all(s, "&amp;", "&");
  replace_all(s, "&lt;", "<");
  replace_all(s, "&gt;", ">");
  replace_all(s, "&quot;", "\"");
  replace_all(s, "&apos;", "'");
  return s;
}

std::vector<std::string> parse_shared_strings(zip_t* archive) {
  std::vector<std::string> out;
  const auto xml = read_zip_entry(archive, "xl/sharedStrings.xml");
  if (xml.empty()) {
    return out;
  }

  std::size_t pos = 0;
  while ((pos = xml.find("<si", pos)) != std::string::npos) {
    const auto si_end = xml.find("</si>", pos);
    if (si_end == std::string::npos) {
      break;
    }
    const auto si_block = xml.substr(pos, si_end - pos);
    std::size_t tpos = 0;
    std::string joined;
    while ((tpos = si_block.find("<t", tpos)) != std::string::npos) {
      const auto t_open_end = si_block.find('>', tpos);
      if (t_open_end == std::string::npos) {
        break;
      }
      const auto t_close = si_block.find("</t>", t_open_end);
      if (t_close == std::string::npos) {
        break;
      }
      joined += si_block.substr(t_open_end + 1, t_close - t_open_end - 1);
      tpos = t_close + 4;
    }
    out.push_back(xml_entity_decode(joined));
    pos = si_end + 5;
  }

  return out;
}

struct SheetRef {
  std::string name;
  std::string rid;
  std::string target;
};

std::vector<SheetRef> parse_sheet_refs(zip_t* archive) {
  std::vector<SheetRef> out;
  const auto workbook_xml = read_zip_entry(archive, "xl/workbook.xml");
  PLUSS_CHECK(!workbook_xml.empty(), "load_excel: missing xl/workbook.xml");

  std::map<std::string, std::string> rid_to_target;
  const auto rels_xml = read_zip_entry(archive, "xl/_rels/workbook.xml.rels");
  if (!rels_xml.empty()) {
    std::size_t pos = 0;
    while ((pos = rels_xml.find("<Relationship", pos)) != std::string::npos) {
      const auto end = rels_xml.find('>', pos);
      if (end == std::string::npos) break;
      const auto tag = std::string_view(rels_xml).substr(pos, end - pos + 1);
      const auto id = xml_attr(tag, "Id");
      const auto target = xml_attr(tag, "Target");
      if (id.has_value() && target.has_value()) {
        rid_to_target[*id] = *target;
      }
      pos = end + 1;
    }
  }

  std::size_t pos = 0;
  while ((pos = workbook_xml.find("<sheet", pos)) != std::string::npos) {
    const auto end = workbook_xml.find('>', pos);
    if (end == std::string::npos) break;
    const auto tag = std::string_view(workbook_xml).substr(pos, end - pos + 1);
    const auto name = xml_attr(tag, "name");
    const auto rid = xml_attr(tag, "r:id");
    if (name.has_value() && rid.has_value()) {
      std::string target = "worksheets/sheet1.xml";
      const auto it = rid_to_target.find(*rid);
      if (it != rid_to_target.end()) {
        target = it->second;
      }
      if (target.rfind("xl/", 0) != 0) {
        target = "xl/" + target;
      }
      out.push_back(SheetRef{*name, *rid, std::move(target)});
    }
    pos = end + 1;
  }
  return out;
}

std::int64_t col_letters_to_idx(std::string_view letters) {
  std::int64_t out = 0;
  for (const char ch : letters) {
    if (ch >= 'A' && ch <= 'Z') {
      out = out * 26 + (ch - 'A' + 1);
    } else if (ch >= 'a' && ch <= 'z') {
      out = out * 26 + (ch - 'a' + 1);
    }
  }
  return out - 1;
}

std::pair<std::int64_t, std::int64_t> parse_cell_ref(std::string_view ref) {
  std::size_t i = 0;
  while (i < ref.size() && std::isalpha(static_cast<unsigned char>(ref[i]))) {
    ++i;
  }
  const auto col = col_letters_to_idx(ref.substr(0, i));
  std::int64_t row = 0;
  if (i < ref.size()) {
    row = std::stoll(std::string(ref.substr(i))) - 1;
  }
  return {row, col};
}

std::vector<std::vector<std::string>> parse_sheet_table(zip_t* archive,
                                                        const SheetRef& sheet,
                                                        const std::vector<std::string>& shared_strings) {
  const auto xml = read_zip_entry(archive, sheet.target);
  PLUSS_CHECK(!xml.empty(), "load_excel: missing worksheet xml '" + sheet.target + "'");

  std::map<std::int64_t, std::map<std::int64_t, std::string>> grid;
  std::int64_t max_row = -1;
  std::int64_t max_col = -1;

  std::size_t row_pos = 0;
  while ((row_pos = xml.find("<row", row_pos)) != std::string::npos) {
    const auto row_open_end = xml.find('>', row_pos);
    if (row_open_end == std::string::npos) break;
    const auto row_close = xml.find("</row>", row_open_end);
    if (row_close == std::string::npos) break;
    const auto row_block = std::string_view(xml).substr(row_pos, row_close - row_pos);

    std::size_t cpos = 0;
    while ((cpos = row_block.find("<c", cpos)) != std::string::npos) {
      const auto c_open_end = row_block.find('>', cpos);
      if (c_open_end == std::string::npos) break;
      const auto c_close = row_block.find("</c>", c_open_end);
      if (c_close == std::string::npos) break;

      const auto c_open = row_block.substr(cpos, c_open_end - cpos + 1);
      const auto c_body = row_block.substr(c_open_end + 1, c_close - c_open_end - 1);
      const auto ref = xml_attr(c_open, "r");
      const auto type = xml_attr(c_open, "t");
      if (!ref.has_value()) {
        cpos = c_close + 4;
        continue;
      }

      auto [r, c] = parse_cell_ref(*ref);
      max_row = std::max(max_row, r);
      max_col = std::max(max_col, c);

      std::string value;
      const auto v_open = c_body.find("<v>");
      const auto v_close = c_body.find("</v>");
      if (v_open != std::string_view::npos && v_close != std::string_view::npos && v_close > v_open + 3) {
        value = std::string(c_body.substr(v_open + 3, v_close - v_open - 3));
      } else {
        const auto t_open = c_body.find("<t");
        if (t_open != std::string_view::npos) {
          const auto t_open_end = c_body.find('>', t_open);
          const auto t_close = c_body.find("</t>", t_open_end);
          if (t_open_end != std::string_view::npos && t_close != std::string_view::npos && t_close > t_open_end) {
            value = std::string(c_body.substr(t_open_end + 1, t_close - t_open_end - 1));
          }
        }
      }

      value = xml_entity_decode(std::move(value));
      if (type.has_value() && *type == "s") {
        const auto idx = std::stoll(value);
        if (idx >= 0 && static_cast<std::size_t>(idx) < shared_strings.size()) {
          value = shared_strings[static_cast<std::size_t>(idx)];
        } else {
          value.clear();
        }
      }

      grid[r][c] = std::move(value);
      cpos = c_close + 4;
    }

    row_pos = row_close + 6;
  }

  if (max_row < 0 || max_col < 0) {
    return {};
  }

  std::vector<std::vector<std::string>> table(
      static_cast<std::size_t>(max_row + 1),
      std::vector<std::string>(static_cast<std::size_t>(max_col + 1), ""));

  for (const auto& [r, row_map] : grid) {
    for (const auto& [c, value] : row_map) {
      table[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] = value;
    }
  }
  return table;
}

#endif

} // namespace

CsvRawBatch read_excel_raw_batch(std::string_view filepath, const ReadExcelOptions& options) {
#if defined(PLUSSMIC_HAS_LIBZIP) && PLUSSMIC_HAS_LIBZIP
  auto source = detail::resolve_local_or_http_source(filepath, "read_excel_raw_batch");
  const std::string_view local_filepath(source.local_path());
  PLUSS_CHECK(!options.all_sheets, "read_excel_raw_batch: all_sheets=true requires read_excel_workbook_raw_batch");
  PLUSS_CHECK(options.sheet_names.empty(), "read_excel_raw_batch: sheet_names requires read_excel_workbook_raw_batch");
  PLUSS_CHECK(options.sheet_indices.empty(), "read_excel_raw_batch: sheet_indices requires read_excel_workbook_raw_batch");
  PLUSS_CHECK(!options.n_sheets.has_value(), "read_excel_raw_batch: n_sheets requires read_excel_workbook_raw_batch");

  int err = 0;
  zip_t* archive = zip_open(std::string(local_filepath).c_str(), ZIP_RDONLY, &err);
  PLUSS_CHECK(archive != nullptr, "read_excel_raw_batch: failed to open workbook");

  auto guard = std::unique_ptr<zip_t, decltype(&zip_close)>(archive, &zip_close);
  const auto shared_strings = parse_shared_strings(archive);
  const auto sheets = parse_sheet_refs(archive);
  PLUSS_CHECK(!sheets.empty(), "read_excel_raw_batch: workbook has no sheets");

  SheetRef target = sheets.front();
  if (options.sheet_name.has_value()) {
    const auto want = normalize_sheet_name(*options.sheet_name);
    auto it = std::find_if(sheets.begin(), sheets.end(), [&](const SheetRef& s) {
      return normalize_sheet_name(s.name) == want;
    });
    PLUSS_CHECK(it != sheets.end(), "read_excel_raw_batch: unknown sheet name '" + *options.sheet_name + "'");
    target = *it;
  } else if (options.sheet_index.has_value()) {
    const auto idx = *options.sheet_index;
    PLUSS_CHECK(idx >= 0 && static_cast<std::size_t>(idx) < sheets.size(),
                "read_excel_raw_batch: sheet_index out of range");
    target = sheets[static_cast<std::size_t>(idx)];
  }

  auto table = parse_sheet_table(archive, target, shared_strings);
  table = apply_excel_table_policies(std::move(table), options);
  table = apply_excel_header_policies(std::move(table), options);
  PLUSS_CHECK(!table.empty(), "read_excel_raw_batch: no tabular rows after applying Excel policies");
  ReadCsvOptions csv_opts;
  csv_opts.header = options.header;
  csv_opts.header_row = options.header_row;
  return read_csv_records_raw_batch(table, csv_opts);
#else
  (void)filepath;
  (void)options;
  PLUSS_FAIL("read_excel_raw_batch: Excel support not built (libzip not available)");
#endif
}

ExcelRawBatchWorkbook read_excel_workbook_raw_batch(std::string_view filepath,
                                                    const ReadExcelOptions& options) {
#if defined(PLUSSMIC_HAS_LIBZIP) && PLUSSMIC_HAS_LIBZIP
  auto source = detail::resolve_local_or_http_source(filepath, "read_excel_workbook_raw_batch");
  const std::string_view local_filepath(source.local_path());

  int err = 0;
  zip_t* archive = zip_open(std::string(local_filepath).c_str(), ZIP_RDONLY, &err);
  PLUSS_CHECK(archive != nullptr, "read_excel_workbook_raw_batch: failed to open workbook");

  auto guard = std::unique_ptr<zip_t, decltype(&zip_close)>(archive, &zip_close);
  const auto shared_strings = parse_shared_strings(archive);
  const auto sheets = parse_sheet_refs(archive);
  PLUSS_CHECK(!sheets.empty(), "read_excel_workbook_raw_batch: workbook has no sheets");

  std::vector<SheetRef> selected_sheets;
  if (!options.sheet_names.empty()) {
    selected_sheets.reserve(options.sheet_names.size());
    for (const auto& wanted_name : options.sheet_names) {
      const auto wanted = normalize_sheet_name(wanted_name);
      auto it = std::find_if(sheets.begin(), sheets.end(), [&](const SheetRef& s) {
        return normalize_sheet_name(s.name) == wanted;
      });
      PLUSS_CHECK(it != sheets.end(), "read_excel_workbook_raw_batch: unknown sheet name '" + wanted_name + "'", wanted_name, pluss::exception::map_keys(sheets));
      selected_sheets.push_back(*it);
    }
  } else if (!options.sheet_indices.empty()) {
    selected_sheets.reserve(options.sheet_indices.size());
    for (const auto idx : options.sheet_indices) {
      PLUSS_CHECK(idx >= 0 && static_cast<std::size_t>(idx) < sheets.size(),
                  "read_excel_workbook_raw_batch: sheet index out of range");
      selected_sheets.push_back(sheets[static_cast<std::size_t>(idx)]);
    }
  } else {
    selected_sheets = sheets;
  }

  if (options.n_sheets.has_value()) {
    const auto n = *options.n_sheets;
    if (n < selected_sheets.size()) {
      selected_sheets.resize(n);
    }
  }

  ExcelRawBatchWorkbook out;
  out.reserve(selected_sheets.size());

  ReadCsvOptions csv_opts;
  csv_opts.header = options.header;
  csv_opts.header_row = options.header_row;

  for (const auto& s : selected_sheets) {
    auto table = parse_sheet_table(archive, s, shared_strings);
    table = apply_excel_table_policies(std::move(table), options);
    table = apply_excel_header_policies(std::move(table), options);
    PLUSS_CHECK(!table.empty(),
                "read_excel_workbook_raw_batch: sheet '" + s.name + "' has no tabular rows after applying policies");
    out.emplace_back(s.name, read_csv_records_raw_batch(table, csv_opts));
  }
  return out;
#else
  (void)filepath;
  (void)options;
  PLUSS_FAIL("read_excel_workbook_raw_batch: Excel support not built (libzip not available)");
#endif
}

CsvBatch read_excel_batch(std::string_view filepath, const ReadExcelOptions& options) {
#if defined(PLUSSMIC_HAS_LIBZIP) && PLUSSMIC_HAS_LIBZIP
  auto source = detail::resolve_local_or_http_source(filepath, "load_excel");
  const std::string_view local_filepath(source.local_path());
  PLUSS_CHECK(!options.all_sheets, "load_excel: all_sheets=true requires load_excel_workbook");
  PLUSS_CHECK(options.sheet_names.empty(), "load_excel: sheet_names requires load_excel_workbook");
  PLUSS_CHECK(options.sheet_indices.empty(), "load_excel: sheet_indices requires load_excel_workbook");
  PLUSS_CHECK(!options.n_sheets.has_value(), "load_excel: n_sheets requires load_excel_workbook");

  int err = 0;
  zip_t* archive = zip_open(std::string(local_filepath).c_str(), ZIP_RDONLY, &err);
  PLUSS_CHECK(archive != nullptr, "load_excel: failed to open workbook");

  auto guard = std::unique_ptr<zip_t, decltype(&zip_close)>(archive, &zip_close);
  const auto shared_strings = parse_shared_strings(archive);
  const auto sheets = parse_sheet_refs(archive);
  PLUSS_CHECK(!sheets.empty(), "load_excel: workbook has no sheets");

  SheetRef target = sheets.front();
  if (options.sheet_name.has_value()) {
    const auto want = normalize_sheet_name(*options.sheet_name);
    auto it = std::find_if(sheets.begin(), sheets.end(), [&](const SheetRef& s) {
      return normalize_sheet_name(s.name) == want;
    });
    PLUSS_CHECK(it != sheets.end(), "load_excel: unknown sheet name '" + *options.sheet_name + "'");
    target = *it;
  } else if (options.sheet_index.has_value()) {
    const auto idx = *options.sheet_index;
    PLUSS_CHECK(idx >= 0 && static_cast<std::size_t>(idx) < sheets.size(), "load_excel: sheet_index out of range");
    target = sheets[static_cast<std::size_t>(idx)];
  }

  auto table = parse_sheet_table(archive, target, shared_strings);
  table = apply_excel_table_policies(std::move(table), options);
  table = apply_excel_header_policies(std::move(table), options);
  PLUSS_CHECK(!table.empty(), "read_excel_batch: no tabular rows after applying Excel policies");
  ReadCsvOptions csv_opts;
  csv_opts.header = options.header;
  csv_opts.header_row = options.header_row;
  return read_csv_records_batch(table, csv_opts);
#else
  (void)filepath;
  (void)options;
  PLUSS_FAIL("read_excel_batch: Excel support not built (libzip not available)");
#endif
}

ExcelBatchWorkbook read_excel_workbook_batch(
    std::string_view filepath,
    const ReadExcelOptions& options) {
#if defined(PLUSSMIC_HAS_LIBZIP) && PLUSSMIC_HAS_LIBZIP
  auto source = detail::resolve_local_or_http_source(filepath, "read_excel_workbook_batch");
  const std::string_view local_filepath(source.local_path());

  int err = 0;
  zip_t* archive = zip_open(std::string(local_filepath).c_str(), ZIP_RDONLY, &err);
  PLUSS_CHECK(archive != nullptr, "read_excel_workbook_batch: failed to open workbook");

  auto guard = std::unique_ptr<zip_t, decltype(&zip_close)>(archive, &zip_close);
  const auto shared_strings = parse_shared_strings(archive);
  const auto sheets = parse_sheet_refs(archive);
  PLUSS_CHECK(!sheets.empty(), "read_excel_workbook_batch: workbook has no sheets");

  std::vector<SheetRef> selected_sheets;
  if (!options.sheet_names.empty()) {
    selected_sheets.reserve(options.sheet_names.size());
    for (const auto& wanted_name : options.sheet_names) {
      const auto wanted = normalize_sheet_name(wanted_name);
      auto it = std::find_if(sheets.begin(), sheets.end(), [&](const SheetRef& s) {
        return normalize_sheet_name(s.name) == wanted;
      });
      PLUSS_CHECK(it != sheets.end(), "read_excel_workbook_batch: unknown sheet name '" + wanted_name + "'", wanted_name, pluss::exception::map_keys(sheets));
      selected_sheets.push_back(*it);
    }
  } else if (!options.sheet_indices.empty()) {
    selected_sheets.reserve(options.sheet_indices.size());
    for (const auto idx : options.sheet_indices) {
      PLUSS_CHECK(idx >= 0 && static_cast<std::size_t>(idx) < sheets.size(),
                  "read_excel_workbook_batch: sheet index out of range");
      selected_sheets.push_back(sheets[static_cast<std::size_t>(idx)]);
    }
  } else {
    selected_sheets = sheets;
  }

  if (options.n_sheets.has_value()) {
    const auto n = *options.n_sheets;
    if (n < selected_sheets.size()) {
      selected_sheets.resize(n);
    }
  }

  ExcelBatchWorkbook out;
  out.reserve(selected_sheets.size());

  ReadCsvOptions csv_opts;
  csv_opts.header = options.header;
  csv_opts.header_row = options.header_row;

  for (const auto& s : selected_sheets) {
    auto table = parse_sheet_table(archive, s, shared_strings);
    table = apply_excel_table_policies(std::move(table), options);
    table = apply_excel_header_policies(std::move(table), options);
    PLUSS_CHECK(!table.empty(),
                "read_excel_workbook_batch: sheet '" + s.name + "' has no tabular rows after applying policies");
    out.emplace_back(s.name, read_csv_records_batch(table, csv_opts));
  }
  return out;
#else
  (void)filepath;
  (void)options;
  PLUSS_FAIL("read_excel_workbook_batch: Excel support not built (libzip not available)");
#endif
}

pluss::table::DataFrame load_excel(std::string_view filepath, const ReadExcelOptions& options) {
  auto batch = read_excel_batch(filepath, options);
  return pluss::table::DataFrame(std::move(batch.features), std::move(batch.cases));
}

Workbook load_excel_workbook(std::string_view filepath, const ReadExcelOptions& options) {
  auto workbook_batch = read_excel_workbook_batch(filepath, options);
  Workbook out;
  out.reserve(workbook_batch.size());
  for (auto& [sheet_name, batch] : workbook_batch) {
    out.emplace_back(std::move(sheet_name),
                     pluss::table::DataFrame(std::move(batch.features), std::move(batch.cases)));
  }
  return out;
}

void save_excel(const pluss::table::DataFrame& frame,
                std::string_view filepath,
                const WriteExcelOptions& options) {
#if defined(PLUSSMIC_HAS_LIBZIP) && PLUSSMIC_HAS_LIBZIP
  PLUSS_CHECK(frame.defined(), "save_excel: frame is undefined");
  PLUSS_CHECK(!filepath.empty(), "save_excel: filepath is empty");

  int err = 0;
  zip_t* archive = zip_open(std::string(filepath).c_str(), ZIP_CREATE | ZIP_TRUNCATE, &err);
  PLUSS_CHECK(archive != nullptr, "save_excel: failed to create workbook");
  auto guard = std::unique_ptr<zip_t, decltype(&zip_discard)>(archive, &zip_discard);

  const auto sheet_name = options.sheet_name.empty() ? std::string("Sheet1") : options.sheet_name;
  const auto names = frame.features();
  std::vector<pluss::table::Slate> cols;
  cols.reserve(names.size());
  for (const auto& name : names) {
    cols.push_back(slate_for_excel_io(frame.features(name)));
  }
  const bool include_cases = options.add_cases || options.cases;
  const auto case_values = include_cases ? slate_for_excel_io(frame.labels().values()) : pluss::table::Slate{};

  std::string sheet_xml;
  sheet_xml += R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)";
  sheet_xml += R"(<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main"><sheetData>)";

  std::int64_t row_num = 1;
  auto emit_row = [&](const std::vector<std::string>& cells) {
    sheet_xml += pluss::tils::OutputEngine::format(R"(<row r="{}">)", row_num);
    for (std::size_t i = 0; i < cells.size(); ++i) {
      const auto ref = pluss::tils::OutputEngine::format("{}{}", column_letters(static_cast<std::int64_t>(i)), row_num);
      sheet_xml += pluss::tils::OutputEngine::format(R"(<c r="{}" t="inlineStr"><is><t>{}</t></is></c>)", ref, xml_escape(cells[i]));
    }
    sheet_xml += "</row>";
    ++row_num;
  };

  if (options.header) {
    std::vector<std::string> header_cells;
    if (include_cases) {
      header_cells.push_back(options.case_label.value_or("case"));
    }
    header_cells.insert(header_cells.end(), names.begin(), names.end());
    emit_row(header_cells);
  }

  for (std::int64_t r = 0; r < frame.ncases(); ++r) {
    std::vector<std::string> row;
    row.reserve(names.size() + (include_cases ? 1u : 0u));
    if (include_cases) {
      row.push_back(slate_value_for_excel(case_values, r, options.null_rep));
    }
    for (const auto& c : cols) {
      row.push_back(slate_value_for_excel(c, r, options.null_rep));
    }
    emit_row(row);
  }

  sheet_xml += "</sheetData></worksheet>";

  const std::string content_types =
      R"(<?xml version="1.0" encoding="UTF-8"?>)"
      R"(<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">)"
      R"(<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>)"
      R"(<Default Extension="xml" ContentType="application/xml"/>)"
      R"(<Override PartName="/xl/workbook.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"/>)"
      R"(<Override PartName="/xl/worksheets/sheet1.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>)"
      R"(</Types>)";

  const std::string rels_root =
      R"(<?xml version="1.0" encoding="UTF-8"?>)"
      R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)"
      R"(<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="xl/workbook.xml"/>)"
      R"(</Relationships>)";

  const std::string workbook_xml =
      pluss::tils::OutputEngine::format(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
                  R"(<workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" )"
                  R"(xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">)"
                  R"(<sheets><sheet name="{}" sheetId="1" r:id="rId1"/></sheets></workbook>)",
                  xml_escape(sheet_name));

  const std::string workbook_rels =
      R"(<?xml version="1.0" encoding="UTF-8"?>)"
      R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)"
      R"(<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet1.xml"/>)"
      R"(</Relationships>)";

  // Keep source buffers alive until zip_close() finalizes all entries.
  std::vector<std::string> zip_payloads;
  zip_payloads.reserve(5);
  auto add_entry = [&](std::string_view path, std::string payload) {
    zip_payloads.push_back(std::move(payload));
    const auto& stable = zip_payloads.back();
    auto* src = zip_source_buffer(archive, stable.data(), stable.size(), 0);
    PLUSS_CHECK(src != nullptr, "save_excel: failed to create zip source");
    if (zip_file_add(archive, std::string(path).c_str(), src, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8) < 0) {
      zip_source_free(src);
      PLUSS_FAIL("save_excel: failed to add zip entry '" + std::string(path) + "'");
    }
  };

  add_entry("[Content_Types].xml", content_types);
  add_entry("_rels/.rels", rels_root);
  add_entry("xl/workbook.xml", workbook_xml);
  add_entry("xl/_rels/workbook.xml.rels", workbook_rels);
  add_entry("xl/worksheets/sheet1.xml", sheet_xml);

  if (zip_close(archive) < 0) {
    const char* zerr = zip_strerror(archive);
    PLUSS_FAIL("save_excel: failed to finalize workbook: " + std::string(zerr ? zerr : "unknown error"));
  }
  guard.release();
#else
  (void)frame;
  (void)filepath;
  (void)options;
  PLUSS_FAIL("save_excel: Excel support not built (libzip not available)");
#endif
}

} // namespace pluss::loader
