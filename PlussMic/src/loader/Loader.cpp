#include "loader/Loader.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <fmt/format.h>

#include "CodeM/core/Errors.h"
#include "CodeM/core/Half.h"
#include "loader/SourceResolver.h"
#include "pluss/codem/series_factory.h"
#include "pluss/coden/note_factory.h"
#include "pluss/codeh/timeline_factory.h"
#include "table/DataFrame.h"

#ifndef PLUSSMIC_HAS_PARQUET_CPP
#define PLUSSMIC_HAS_PARQUET_CPP 0
#endif

#if PLUSSMIC_HAS_PARQUET_CPP
// Arrow's bytes_view aliases std::basic_string_view<uint8_t>. Some libc++
// toolchains (notably conda clang/libc++ combos) do not provide
// std::char_traits<unsigned char>, so provide a local compatibility
// specialization before including Arrow headers.
#if defined(_LIBCPP_VERSION)
namespace std {
template <>
struct char_traits<unsigned char> {
  using char_type = unsigned char;
  using int_type = unsigned int;
  using off_type = streamoff;
  using pos_type = streampos;
  using state_type = mbstate_t;

  static constexpr void assign(char_type& c1, const char_type& c2) noexcept { c1 = c2; }
  static constexpr bool eq(char_type c1, char_type c2) noexcept { return c1 == c2; }
  static constexpr bool lt(char_type c1, char_type c2) noexcept { return c1 < c2; }

  static int compare(const char_type* s1, const char_type* s2, std::size_t n) noexcept {
    if (n == 0) return 0;
    return std::memcmp(s1, s2, n);
  }

  static std::size_t length(const char_type* s) noexcept {
    std::size_t n = 0;
    while (s[n] != static_cast<char_type>(0)) ++n;
    return n;
  }

  static const char_type* find(const char_type* s, std::size_t n, const char_type& a) noexcept {
    for (std::size_t i = 0; i < n; ++i) {
      if (s[i] == a) return s + i;
    }
    return nullptr;
  }

  static char_type* move(char_type* s1, const char_type* s2, std::size_t n) noexcept {
    if (n == 0) return s1;
    return static_cast<char_type*>(std::memmove(s1, s2, n));
  }

  static char_type* copy(char_type* s1, const char_type* s2, std::size_t n) noexcept {
    if (n == 0) return s1;
    return static_cast<char_type*>(std::memcpy(s1, s2, n));
  }

  static char_type* assign(char_type* s, std::size_t n, char_type a) noexcept {
    for (std::size_t i = 0; i < n; ++i) s[i] = a;
    return s;
  }

  static constexpr int_type not_eof(int_type c) noexcept { return eq_int_type(c, eof()) ? ~eof() : c; }
  static constexpr char_type to_char_type(int_type c) noexcept { return static_cast<char_type>(c); }
  static constexpr int_type to_int_type(char_type c) noexcept { return static_cast<int_type>(c); }
  static constexpr bool eq_int_type(int_type c1, int_type c2) noexcept { return c1 == c2; }
  static constexpr int_type eof() noexcept { return static_cast<int_type>(EOF); }
};
}  // namespace std
#endif  // defined(_LIBCPP_VERSION)

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/table.h>
#include <parquet/arrow/reader.h>
#include <parquet/arrow/writer.h>
#endif

namespace pluss::loader {

namespace {

std::string trim_ascii(std::string_view s) {
  std::size_t b = 0;
  std::size_t e = s.size();
  while (b < e && std::isspace(static_cast<unsigned char>(s[b])) != 0) ++b;
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])) != 0) --e;
  return std::string(s.substr(b, e - b));
}

std::string lower_copy(std::string s) {
  for (auto& ch : s) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return s;
}

std::string normalize_key(std::string key) {
  key = lower_copy(std::move(key));
  std::string out;
  out.reserve(key.size());
  for (const char ch : key) {
    if (std::isspace(static_cast<unsigned char>(ch)) != 0 || ch == '_' || ch == '-') {
      continue;
    }
    out.push_back(ch);
  }
  return out;
}

const ConfigValue* find_config(const Config& config, std::initializer_list<std::string_view> aliases) {
  for (const auto alias : aliases) {
    const auto needle = normalize_key(std::string(alias));
    for (const auto& [k, v] : config) {
      if (normalize_key(k) == needle) {
        return &v;
      }
    }
  }
  return nullptr;
}

std::optional<bool> parse_bool_value(const ConfigValue& value) {
  if (auto b = value.as_bool(); b.has_value()) return b;
  if (auto i = value.as_int(); i.has_value()) return *i != 0;
  if (auto s = value.as_string(); s.has_value()) {
    const auto v = lower_copy(trim_ascii(*s));
    if (v == "true" || v == "yes" || v == "1" || v == "on") return true;
    if (v == "false" || v == "no" || v == "0" || v == "off" || v == "none") return false;
  }
  return std::nullopt;
}

std::optional<std::int64_t> parse_int_value(const ConfigValue& value) {
  if (auto i = value.as_int(); i.has_value()) return i;
  if (auto d = value.as_double(); d.has_value()) return static_cast<std::int64_t>(*d);
  if (auto s = value.as_string(); s.has_value()) {
    const auto t = trim_ascii(*s);
    if (t.empty()) return std::nullopt;
    try {
      return std::stoll(t);
    } catch (...) {
      return std::nullopt;
    }
  }
  return std::nullopt;
}

std::optional<std::string> parse_string_value(const ConfigValue& value) {
  if (auto s = value.as_string(); s.has_value()) return std::string(*s);
  return std::nullopt;
}

std::optional<std::vector<std::string>> parse_string_list_value(const ConfigValue& value) {
  if (auto v = value.as_string_list(); v.has_value()) return v;
  if (auto s = value.as_string(); s.has_value()) {
    std::vector<std::string> out;
    const std::string text(*s);
    std::size_t pos = 0;
    while (pos <= text.size()) {
      auto comma = text.find(',', pos);
      if (comma == std::string::npos) comma = text.size();
      const auto part = trim_ascii(std::string_view(text).substr(pos, comma - pos));
      if (!part.empty()) {
        out.push_back(part);
      }
      if (comma == text.size()) break;
      pos = comma + 1;
    }
    return out;
  }
  return std::nullopt;
}

std::optional<std::vector<std::size_t>> parse_size_t_list_value(const ConfigValue& value);

bool is_none_token(std::string_view s) {
  const auto v = lower_copy(trim_ascii(s));
  return v.empty() || v == "none" || v == "null";
}

char parse_sep_token(std::string_view token, char fallback) {
  const auto v = lower_copy(trim_ascii(token));
  if (v == "comma") return ',';
  if (v == "semicolon" || v == "semi-colon" || v == "semi") return ';';
  if (v == "tab" || v == "\\t") return '\t';
  if (v == "pipe") return '|';
  if (v.size() == 1) return v[0];
  return fallback;
}

ReadCsvOptions csv_read_options_from_config(const Config& config) {
  ReadCsvOptions options;

  if (const auto* v = find_config(config, {"sep", "delimiter", "separator"})) {
    if (auto s = parse_string_value(*v); s.has_value()) {
      options.sep = parse_sep_token(*s, options.sep);
      options.auto_detect_sep = false;
    }
  }

  if (const auto* v = find_config(config, {"ncases"})) {
    if (auto n = parse_int_value(*v); n.has_value() && *n >= 0) {
      options.ncases = static_cast<std::size_t>(*n);
    }
  }

  if (const auto* v = find_config(config, {"casecol", "case", "caseindex"})) {
    if (auto n = parse_int_value(*v); n.has_value() && *n >= 0) {
      options.case_col_idx = static_cast<std::size_t>(*n);
    } else if (auto s = parse_string_value(*v); s.has_value()) {
      if (!is_none_token(*s)) {
        options.case_col = *s;
      }
    }
  }

  if (const auto* v = find_config(config, {"usefeats", "features", "selectfeats"})) {
    if (auto feats = parse_string_list_value(*v); feats.has_value()) {
      options.usefeats = std::move(*feats);
    }
  }

  if (const auto* v = find_config(config,
                                  {"usefeatsidx", "featuresidx", "selectfeatsidx"})) {
    if (auto idx = parse_size_t_list_value(*v); idx.has_value()) {
      options.usefeats_idx = std::move(*idx);
    }
  }

  if (const auto* v = find_config(config, {"onbadlines", "badlines"})) {
    if (auto s = parse_string_value(*v); s.has_value()) {
      const auto low = lower_copy(trim_ascii(*s));
      if (low == "warn") {
        options.on_bad_lines = BadLinePolicy::Warn;
      } else if (low == "skip") {
        options.on_bad_lines = BadLinePolicy::Skip;
      } else {
        options.on_bad_lines = BadLinePolicy::Error;
      }
    }
  }

  if (const auto* v = find_config(config, {"header", "colnames", "columnnames"})) {
    if (auto b = parse_bool_value(*v); b.has_value()) {
      options.header = *b;
    } else if (auto s = parse_string_value(*v); s.has_value()) {
      if (is_none_token(*s)) {
        options.header = false;
      }
    } else if (auto names = parse_string_list_value(*v); names.has_value() && !names->empty()) {
      options.names = std::move(*names);
      options.header = false;
    }
  }

  if (const auto* v = find_config(config, {"auto_detect_sep", "autosep"})) {
    if (auto b = parse_bool_value(*v); b.has_value()) {
      options.auto_detect_sep = *b;
    }
  }

  if (const auto* v = find_config(config, {"engine", "csvengine"})) {
    if (auto s = parse_string_value(*v); s.has_value()) {
      const auto low = lower_copy(trim_ascii(*s));
      if (low == "parallel") {
        options.engine = CsvEngine::Parallel;
      } else if (low == "serial") {
        options.engine = CsvEngine::Serial;
      } else {
        options.engine = CsvEngine::Auto;
      }
    }
  }

  if (const auto* v = find_config(config, {"threads", "parallelthreads", "numthreads"})) {
    if (auto n = parse_int_value(*v); n.has_value() && *n > 0) {
      options.parallel_threads = static_cast<std::size_t>(*n);
    }
  }

  if (const auto* v = find_config(config, {"parallelminbytes", "parallelthreshold"})) {
    if (auto n = parse_int_value(*v); n.has_value() && *n > 0) {
      options.parallel_min_bytes = static_cast<std::size_t>(*n);
    }
  }

  if (const auto* v = find_config(config, {"profile", "csvprofile"})) {
    if (auto b = parse_bool_value(*v); b.has_value()) {
      options.profile = *b;
    }
  }

  return options;
}

ReadExcelOptions excel_read_options_from_config(const Config& config) {
  ReadExcelOptions options;

  if (const auto* v = find_config(config, {"header", "colnames", "columnnames"})) {
    if (auto b = parse_bool_value(*v); b.has_value()) {
      options.header = *b;
    } else if (auto s = parse_string_value(*v); s.has_value() && is_none_token(*s)) {
      options.header = false;
    }
  }

  if (const auto* v = find_config(config, {"headerrow", "header_row"})) {
    if (auto n = parse_int_value(*v); n.has_value()) {
      PLUSS_CHECK(*n >= 0, "loader::load_excel: header_row must be >= 0");
      options.header_row = *n;
    }
  }

  if (const auto* v = find_config(config, {"headerrows", "header_rows", "headerdepth"})) {
    if (auto n = parse_int_value(*v); n.has_value()) {
      PLUSS_CHECK(*n > 0, "loader::load_excel: header_rows must be > 0");
      options.header_rows = static_cast<std::size_t>(*n);
    }
  }

  if (const auto* v = find_config(config, {"headerffill", "header_forward_fill", "ffill_header"})) {
    if (auto b = parse_bool_value(*v); b.has_value()) {
      options.header_ffill = *b;
    }
  }

  if (const auto* v = find_config(config, {"headerjoiner", "header_joiner", "headersep", "header_sep"})) {
    if (auto s = parse_string_value(*v); s.has_value()) {
      options.header_joiner = *s;
    }
  }

  if (const auto* v = find_config(config, {"sheetname", "sheet", "sheetindex"})) {
    if (v->is_none()) {
      options.all_sheets = true;
    } else if (auto idx = parse_int_value(*v); idx.has_value()) {
      options.sheet_index = *idx;
    } else if (auto names = v->as_string_list(); names.has_value() && !names->empty()) {
      options.sheet_names = std::move(*names);
      options.all_sheets = false;
      options.sheet_name.reset();
      options.sheet_index.reset();
    } else if (auto s = parse_string_value(*v); s.has_value()) {
      if (is_none_token(*s)) {
        options.all_sheets = true;
      } else if (s->find(',') != std::string::npos) {
        if (auto names = parse_string_list_value(*v); names.has_value() && !names->empty()) {
          options.sheet_names = std::move(*names);
          options.all_sheets = false;
          options.sheet_name.reset();
          options.sheet_index.reset();
        }
      } else {
        options.sheet_name = *s;
        options.all_sheets = false;
        options.sheet_names.clear();
        options.sheet_index.reset();
      }
    }
  }

  if (const auto* v = find_config(config, {"allsheets", "workbook"})) {
    if (auto b = parse_bool_value(*v); b.has_value()) {
      options.all_sheets = *b;
    }
  }

  if (const auto* v = find_config(config, {"nsheets", "n sheets", "numsheets"})) {
    if (auto n = parse_int_value(*v); n.has_value() && *n > 0) {
      options.n_sheets = static_cast<std::size_t>(*n);
    }
  }

  if (const auto* v = find_config(config, {"startrow", "rowstart", "firstrow"})) {
    if (auto n = parse_int_value(*v); n.has_value()) {
      PLUSS_CHECK(*n >= 0, "loader::load_excel: start_row must be >= 0");
      options.start_row = *n;
    }
  }

  if (const auto* v = find_config(config, {"endrow", "rowend", "lastrow"})) {
    if (auto n = parse_int_value(*v); n.has_value()) {
      PLUSS_CHECK(*n >= 0, "loader::load_excel: end_row must be >= 0");
      options.end_row = *n;
    }
  }

  if (const auto* v = find_config(config, {"startcol", "colstart", "firstcol"})) {
    if (auto n = parse_int_value(*v); n.has_value()) {
      PLUSS_CHECK(*n >= 0, "loader::load_excel: start_col must be >= 0");
      options.start_col = *n;
    }
  }

  if (const auto* v = find_config(config, {"endcol", "colend", "lastcol"})) {
    if (auto n = parse_int_value(*v); n.has_value()) {
      PLUSS_CHECK(*n >= 0, "loader::load_excel: end_col must be >= 0");
      options.end_col = *n;
    }
  }

  if (const auto* v = find_config(config, {"trimouterempty", "trimempty", "trim"})) {
    if (auto b = parse_bool_value(*v); b.has_value()) {
      options.trim_outer_empty = *b;
    }
  }

  if (const auto* v = find_config(config, {"autotableblock", "autotable", "infertableblock"})) {
    if (auto b = parse_bool_value(*v); b.has_value()) {
      options.auto_table_block = *b;
    }
  }

  if (const auto* v = find_config(config, {"autominnonemptycells", "tableminnonemptycells"})) {
    if (auto n = parse_int_value(*v); n.has_value() && *n > 0) {
      options.auto_min_non_empty_cells = static_cast<std::size_t>(*n);
    }
  }

  if (const auto* v = find_config(config, {"automaxblankgap", "tablemaxblankgap"})) {
    if (auto n = parse_int_value(*v); n.has_value() && *n >= 0) {
      options.auto_max_blank_gap = static_cast<std::size_t>(*n);
    }
  }

  if (options.start_row.has_value() && options.end_row.has_value()) {
    PLUSS_CHECK(*options.start_row <= *options.end_row,
                "loader::load_excel: start_row cannot be greater than end_row");
  }
  if (options.start_col.has_value() && options.end_col.has_value()) {
    PLUSS_CHECK(*options.start_col <= *options.end_col,
                "loader::load_excel: start_col cannot be greater than end_col");
  }

  return options;
}

pluss::table::Thread infer_padded_thread_from_raw(const CsvRawColumn& column,
                                                  const ReadCsvOptions& options,
                                                  std::int64_t target_len) {
  std::vector<std::string> values = column.values;
  if (target_len >= 0 && static_cast<std::int64_t>(values.size()) < target_len) {
    values.resize(static_cast<std::size_t>(target_len));
  }
  return infer_thread_from_raw_csv_column(column.name, column.source_index, values, options);
}

pluss::table::DataFrame dataframe_from_raw_batch_padded(const CsvRawBatch& raw,
                                                        const ReadCsvOptions& options) {
  std::int64_t target_len = 0;
  for (const auto& col : raw.features) {
    target_len = std::max<std::int64_t>(target_len, static_cast<std::int64_t>(col.values.size()));
  }
  if (raw.cases.has_value()) {
    target_len = std::max<std::int64_t>(target_len, static_cast<std::int64_t>(raw.cases->values.size()));
  }

  std::vector<std::pair<std::string, pluss::table::Thread>> cols;
  cols.reserve(raw.features.size());
  for (const auto& col : raw.features) {
    cols.emplace_back(col.name, infer_padded_thread_from_raw(col, options, target_len));
  }

  std::optional<pluss::table::Thread> cases = std::nullopt;
  if (raw.cases.has_value()) {
    cases = infer_padded_thread_from_raw(*raw.cases, options, target_len);
  }

  return pluss::table::DataFrame(std::move(cols), std::move(cases));
}

enum class TxtParseMode : std::uint8_t {
  Auto = 0,
  Delimited = 1,
  Whitespace = 2,
  FixedWidth = 3,
};

TxtParseMode txt_parse_mode_from_config(const Config& config) {
  if (const auto* v = find_config(config, {"txtmode", "textmode", "txt_parser", "parser"})) {
    if (auto s = parse_string_value(*v); s.has_value()) {
      const auto low = normalize_key(*s);
      if (low == "delimited" || low == "csv") return TxtParseMode::Delimited;
      if (low == "whitespace" || low == "whitespacedelimited" || low == "space") return TxtParseMode::Whitespace;
      if (low == "fixedwidth" || low == "fwf" || low == "fixed") return TxtParseMode::FixedWidth;
    }
  }
  if (const auto* v = find_config(config, {"delim_whitespace", "whitespace"})) {
    if (auto b = parse_bool_value(*v); b.has_value() && *b) {
      return TxtParseMode::Whitespace;
    }
  }
  if (const auto* v = find_config(config, {"fixed_width", "fwf"})) {
    if (auto b = parse_bool_value(*v); b.has_value() && *b) {
      return TxtParseMode::FixedWidth;
    }
  }
  if (find_config(config, {"widths", "colwidths", "fixedwidths", "fwf_widths"}) != nullptr) {
    return TxtParseMode::FixedWidth;
  }
  return TxtParseMode::Auto;
}

std::vector<std::size_t> parse_widths_config(const Config& config) {
  const auto* v = find_config(config, {"widths", "colwidths", "fixedwidths", "fwf_widths"});
  if (v == nullptr) return {};

  std::vector<std::string> tokens;
  if (auto list = parse_string_list_value(*v); list.has_value()) {
    tokens = std::move(*list);
  } else if (auto s = parse_string_value(*v); s.has_value()) {
    std::string t = *s;
    for (auto& ch : t) {
      if (ch == ',' || ch == ';' || ch == '|') ch = ' ';
    }
    std::istringstream iss(t);
    std::string token;
    while (iss >> token) {
      tokens.push_back(std::move(token));
    }
  }

  std::vector<std::size_t> widths;
  widths.reserve(tokens.size());
  for (const auto& token : tokens) {
    try {
      const auto parsed = std::stoll(trim_ascii(token));
      PLUSS_CHECK(parsed > 0, "loader::read_txt_raw_batch: fixed-width values must be positive");
      widths.push_back(static_cast<std::size_t>(parsed));
    } catch (...) {
      PLUSS_FAIL("loader::read_txt_raw_batch: invalid fixed width token '" + token + "'");
    }
  }
  return widths;
}

bool is_ascii_space(char ch) {
  return std::isspace(static_cast<unsigned char>(ch)) != 0;
}

std::vector<std::string> split_by_min_gap(std::string_view line, std::size_t min_gap) {
  std::vector<std::string> out;
  const auto n = line.size();
  std::size_t i = 0;
  while (i < n) {
    while (i < n && is_ascii_space(line[i])) ++i;
    if (i >= n) break;

    const auto start = i;
    while (i < n) {
      if (!is_ascii_space(line[i])) {
        ++i;
        continue;
      }
      std::size_t j = i;
      while (j < n && is_ascii_space(line[j])) ++j;
      if (j - i >= min_gap) {
        break;
      }
      i = j;
    }

    std::size_t end = i;
    while (end > start && is_ascii_space(line[end - 1])) --end;
    out.emplace_back(line.substr(start, end - start));
    while (i < n && is_ascii_space(line[i])) ++i;
  }
  return out;
}

std::vector<std::string> split_fixed_width_line(std::string_view line,
                                                const std::vector<std::size_t>& widths) {
  std::vector<std::string> out;
  out.reserve(widths.size() + 1);
  std::size_t pos = 0;
  for (const auto width : widths) {
    if (pos >= line.size()) {
      out.emplace_back("");
      pos += width;
      continue;
    }
    const auto len = std::min<std::size_t>(width, line.size() - pos);
    out.emplace_back(trim_ascii(line.substr(pos, len)));
    pos += width;
  }
  if (pos < line.size()) {
    out.emplace_back(trim_ascii(line.substr(pos)));
  }
  return out;
}

std::vector<std::vector<std::string>> read_text_records_with_mode(std::string_view filepath,
                                                                  std::size_t min_gap,
                                                                  const std::vector<std::size_t>& fixed_widths,
                                                                  bool skip_blank_lines) {
  std::ifstream in(std::string(filepath), std::ios::in | std::ios::binary);
  PLUSS_CHECK(in.is_open(), "loader::read_txt_raw_batch: failed to open file '" + std::string(filepath) + "'");

  std::vector<std::vector<std::string>> records;
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    std::vector<std::string> cells = fixed_widths.empty()
        ? split_by_min_gap(line, min_gap)
        : split_fixed_width_line(line, fixed_widths);
    if (cells.empty() && skip_blank_lines) {
      continue;
    }
    records.push_back(std::move(cells));
  }
  return records;
}

void pad_records_to_rectangular(std::vector<std::vector<std::string>>& records) {
  std::size_t max_cols = 0;
  for (const auto& row : records) {
    max_cols = std::max(max_cols, row.size());
  }
  for (auto& row : records) {
    row.resize(max_cols);
  }
}

double score_record_grid(const std::vector<std::vector<std::string>>& records) {
  if (records.empty()) return -1.0;
  std::unordered_map<std::size_t, std::size_t> freq;
  std::size_t non_empty = 0;
  std::size_t total_cols = 0;
  for (const auto& row : records) {
    if (row.empty()) continue;
    ++non_empty;
    total_cols += row.size();
    ++freq[row.size()];
  }
  if (non_empty == 0) return -1.0;
  std::size_t mode_cols = 0;
  std::size_t mode_count = 0;
  for (const auto& [cols, count] : freq) {
    if (count > mode_count || (count == mode_count && cols > mode_cols)) {
      mode_cols = cols;
      mode_count = count;
    }
  }
  if (mode_cols <= 1) return 0.0;
  const auto consistency = static_cast<double>(mode_count) / static_cast<double>(non_empty);
  const auto avg_cols = static_cast<double>(total_cols) / static_cast<double>(non_empty);
  return consistency * 100.0 + avg_cols;
}

CsvRawBatch parse_txt_records_as_raw_batch(const std::vector<std::vector<std::string>>& records,
                                           ReadCsvOptions options) {
  std::vector<std::vector<std::string>> padded = records;
  pad_records_to_rectangular(padded);
  return read_csv_records_raw_batch(padded, options);
}

class JsonValue {
public:
  enum class Kind : std::uint8_t {
    Null = 0,
    Bool = 1,
    Number = 2,
    String = 3,
    Array = 4,
    Object = 5,
  };

  Kind kind{Kind::Null};
  bool bool_value{false};
  std::string text{};
  std::vector<JsonValue> array{};
  std::vector<std::pair<std::string, JsonValue>> object{};
};

class JsonParser final {
public:
  explicit JsonParser(std::string_view text) : text_(text) {}

  JsonValue parse() {
    skip_ws();
    auto out = parse_value();
    skip_ws();
    PLUSS_CHECK(pos_ == text_.size(), "loader::read_json_raw_batch: trailing characters after JSON root");
    return out;
  }

private:
  char peek() const {
    if (pos_ >= text_.size()) return '\0';
    return text_[pos_];
  }

  char get() {
    PLUSS_CHECK(pos_ < text_.size(), "loader::read_json_raw_batch: unexpected EOF");
    return text_[pos_++];
  }

  void expect_char(char ch, const char* where) {
    const auto got = get();
    PLUSS_CHECK(got == ch, std::string("loader::read_json_raw_batch: expected '") + ch + "' in " + where + "'");
  }

  void skip_ws() {
    while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_])) != 0) ++pos_;
  }

  JsonValue parse_value() {
    skip_ws();
    const auto ch = peek();
    if (ch == '"') return parse_string_value();
    if (ch == '{') return parse_object();
    if (ch == '[') return parse_array();
    if (ch == 't') return parse_true();
    if (ch == 'f') return parse_false();
    if (ch == 'n') return parse_null();
    if (ch == '-' || std::isdigit(static_cast<unsigned char>(ch)) != 0) return parse_number();
    PLUSS_FAIL("loader::read_json_raw_batch: invalid JSON token");
  }

  JsonValue parse_null() {
    expect_keyword("null");
    return JsonValue{};
  }

  JsonValue parse_true() {
    expect_keyword("true");
    JsonValue out;
    out.kind = JsonValue::Kind::Bool;
    out.bool_value = true;
    return out;
  }

  JsonValue parse_false() {
    expect_keyword("false");
    JsonValue out;
    out.kind = JsonValue::Kind::Bool;
    out.bool_value = false;
    return out;
  }

  void expect_keyword(std::string_view kw) {
    PLUSS_CHECK(pos_ + kw.size() <= text_.size(), "loader::read_json_raw_batch: unexpected EOF in keyword");
    for (std::size_t i = 0; i < kw.size(); ++i) {
      PLUSS_CHECK(text_[pos_ + i] == kw[i], "loader::read_json_raw_batch: invalid keyword");
    }
    pos_ += kw.size();
  }

  static int hex_digit(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return 10 + (ch - 'a');
    if (ch >= 'A' && ch <= 'F') return 10 + (ch - 'A');
    return -1;
  }

  static void append_utf8_codepoint(std::string& out, std::uint32_t cp) {
    if (cp <= 0x7F) {
      out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
      out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
      out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
      out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
      out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
      out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
      out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
      out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
      out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
      out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
  }

  std::string parse_string() {
    expect_char('"', "string");
    std::string out;
    while (true) {
      PLUSS_CHECK(pos_ < text_.size(), "loader::read_json_raw_batch: unterminated JSON string");
      const auto ch = get();
      if (ch == '"') {
        break;
      }
      if (ch != '\\') {
        out.push_back(ch);
        continue;
      }

      PLUSS_CHECK(pos_ < text_.size(), "loader::read_json_raw_batch: invalid escape at EOF");
      const auto esc = get();
      switch (esc) {
        case '"': out.push_back('"'); break;
        case '\\': out.push_back('\\'); break;
        case '/': out.push_back('/'); break;
        case 'b': out.push_back('\b'); break;
        case 'f': out.push_back('\f'); break;
        case 'n': out.push_back('\n'); break;
        case 'r': out.push_back('\r'); break;
        case 't': out.push_back('\t'); break;
        case 'u': {
          PLUSS_CHECK(pos_ + 4 <= text_.size(), "loader::read_json_raw_batch: short \\u escape");
          std::uint32_t cp = 0;
          for (std::size_t i = 0; i < 4; ++i) {
            const auto v = hex_digit(text_[pos_ + i]);
            PLUSS_CHECK(v >= 0, "loader::read_json_raw_batch: invalid \\u escape");
            cp = (cp << 4U) | static_cast<std::uint32_t>(v);
          }
          pos_ += 4;
          append_utf8_codepoint(out, cp);
          break;
        }
        default:
          PLUSS_FAIL("loader::read_json_raw_batch: unsupported escape sequence");
      }
    }
    return out;
  }

  JsonValue parse_string_value() {
    JsonValue out;
    out.kind = JsonValue::Kind::String;
    out.text = parse_string();
    return out;
  }

  JsonValue parse_number() {
    const auto start = pos_;
    if (peek() == '-') ++pos_;

    PLUSS_CHECK(pos_ < text_.size(), "loader::read_json_raw_batch: invalid numeric token");
    if (peek() == '0') {
      ++pos_;
    } else {
      PLUSS_CHECK(std::isdigit(static_cast<unsigned char>(peek())) != 0,
                  "loader::read_json_raw_batch: invalid numeric token");
      while (std::isdigit(static_cast<unsigned char>(peek())) != 0) ++pos_;
    }

    if (peek() == '.') {
      ++pos_;
      PLUSS_CHECK(std::isdigit(static_cast<unsigned char>(peek())) != 0,
                  "loader::read_json_raw_batch: invalid numeric fraction");
      while (std::isdigit(static_cast<unsigned char>(peek())) != 0) ++pos_;
    }

    if (peek() == 'e' || peek() == 'E') {
      ++pos_;
      if (peek() == '+' || peek() == '-') ++pos_;
      PLUSS_CHECK(std::isdigit(static_cast<unsigned char>(peek())) != 0,
                  "loader::read_json_raw_batch: invalid numeric exponent");
      while (std::isdigit(static_cast<unsigned char>(peek())) != 0) ++pos_;
    }

    JsonValue out;
    out.kind = JsonValue::Kind::Number;
    out.text = std::string(text_.substr(start, pos_ - start));
    return out;
  }

  JsonValue parse_array() {
    expect_char('[', "array");
    JsonValue out;
    out.kind = JsonValue::Kind::Array;
    skip_ws();
    if (peek() == ']') {
      ++pos_;
      return out;
    }
    while (true) {
      out.array.push_back(parse_value());
      skip_ws();
      if (peek() == ']') {
        ++pos_;
        break;
      }
      expect_char(',', "array");
    }
    return out;
  }

  JsonValue parse_object() {
    expect_char('{', "object");
    JsonValue out;
    out.kind = JsonValue::Kind::Object;
    skip_ws();
    if (peek() == '}') {
      ++pos_;
      return out;
    }
    while (true) {
      skip_ws();
      PLUSS_CHECK(peek() == '"', "loader::read_json_raw_batch: object key must be a string");
      auto key = parse_string();
      skip_ws();
      expect_char(':', "object");
      auto value = parse_value();
      out.object.emplace_back(std::move(key), std::move(value));
      skip_ws();
      if (peek() == '}') {
        ++pos_;
        break;
      }
      expect_char(',', "object");
    }
    return out;
  }

  std::string_view text_;
  std::size_t pos_{0};
};

std::string json_escape_text(std::string_view value) {
  std::string out;
  out.reserve(value.size() + 8);
  for (const auto ch : value) {
    switch (ch) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\b': out += "\\b"; break;
      case '\f': out += "\\f"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out.push_back(ch); break;
    }
  }
  return out;
}

std::string json_compact_text(const JsonValue& value);

std::string json_scalar_token(const JsonValue& value) {
  switch (value.kind) {
    case JsonValue::Kind::Null:
      return "";
    case JsonValue::Kind::Bool:
      return value.bool_value ? "true" : "false";
    case JsonValue::Kind::Number:
    case JsonValue::Kind::String:
      return value.text;
    case JsonValue::Kind::Array:
    case JsonValue::Kind::Object:
      return json_compact_text(value);
  }
  return "";
}

std::string json_compact_text(const JsonValue& value) {
  switch (value.kind) {
    case JsonValue::Kind::Null:
      return "null";
    case JsonValue::Kind::Bool:
      return value.bool_value ? "true" : "false";
    case JsonValue::Kind::Number:
      return value.text;
    case JsonValue::Kind::String:
      return "\"" + json_escape_text(value.text) + "\"";
    case JsonValue::Kind::Array: {
      std::string out = "[";
      for (std::size_t i = 0; i < value.array.size(); ++i) {
        if (i != 0) out += ",";
        out += json_compact_text(value.array[i]);
      }
      out += "]";
      return out;
    }
    case JsonValue::Kind::Object: {
      std::string out = "{";
      for (std::size_t i = 0; i < value.object.size(); ++i) {
        if (i != 0) out += ",";
        out += "\"" + json_escape_text(value.object[i].first) + "\":" + json_compact_text(value.object[i].second);
      }
      out += "}";
      return out;
    }
  }
  return "null";
}

const JsonValue* object_find(const JsonValue& object, std::string_view key) {
  if (object.kind != JsonValue::Kind::Object) return nullptr;
  for (const auto& [k, v] : object.object) {
    if (k == key) return &v;
  }
  return nullptr;
}

std::optional<std::vector<std::size_t>> parse_size_t_list_value(const ConfigValue& value) {
  std::vector<std::string> tokens;
  if (auto list = value.as_string_list(); list.has_value()) {
    tokens = std::move(*list);
  } else if (auto s = value.as_string(); s.has_value()) {
    std::string text(*s);
    for (auto& ch : text) {
      if (ch == ',' || ch == ';' || ch == '|') ch = ' ';
    }
    std::istringstream iss(text);
    std::string token;
    while (iss >> token) {
      tokens.push_back(std::move(token));
    }
  } else if (auto i = value.as_int(); i.has_value()) {
    tokens.push_back(std::to_string(*i));
  }

  if (tokens.empty()) return std::nullopt;
  std::vector<std::size_t> out;
  out.reserve(tokens.size());
  for (const auto& token : tokens) {
    try {
      const auto parsed = std::stoll(trim_ascii(token));
      if (parsed < 0) return std::nullopt;
      out.push_back(static_cast<std::size_t>(parsed));
    } catch (...) {
      return std::nullopt;
    }
  }
  return out;
}

bool all_array_items_objects(const JsonValue& value) {
  if (value.kind != JsonValue::Kind::Array) return false;
  for (const auto& item : value.array) {
    if (item.kind != JsonValue::Kind::Object) return false;
  }
  return true;
}

bool all_object_values_are_arrays_or_scalars(const JsonValue& value) {
  if (value.kind != JsonValue::Kind::Object) return false;
  for (const auto& [_, v] : value.object) {
    if (v.kind == JsonValue::Kind::Array) continue;
    if (v.kind == JsonValue::Kind::Null ||
        v.kind == JsonValue::Kind::Bool ||
        v.kind == JsonValue::Kind::Number ||
        v.kind == JsonValue::Kind::String) {
      continue;
    }
    return false;
  }
  return true;
}

std::vector<std::string> column_names_from_config(const Config& config) {
  if (const auto* v = find_config(config, {"columns", "names", "column_names"})) {
    if (auto list = parse_string_list_value(*v); list.has_value()) {
      return *list;
    }
  }
  return {};
}

CsvRawBatch raw_from_json_records_array(const JsonValue& records) {
  PLUSS_CHECK(records.kind == JsonValue::Kind::Array, "loader::read_json_raw_batch: expected records array");
  std::vector<std::string> names;
  std::unordered_map<std::string, std::size_t> name_to_idx;

  for (const auto& row : records.array) {
    PLUSS_CHECK(row.kind == JsonValue::Kind::Object, "loader::read_json_raw_batch: records must be JSON objects");
    for (const auto& [name, _] : row.object) {
      if (name_to_idx.find(name) == name_to_idx.end()) {
        name_to_idx.emplace(name, names.size());
        names.push_back(name);
      }
    }
  }

  CsvRawBatch raw;
  raw.features.reserve(names.size());
  const auto nrows = records.array.size();
  for (std::size_t i = 0; i < names.size(); ++i) {
    raw.features.push_back(CsvRawColumn{
        names[i],
        i,
        std::vector<std::string>(nrows),
    });
  }

  for (std::size_t row_idx = 0; row_idx < records.array.size(); ++row_idx) {
    const auto& row = records.array[row_idx];
    for (const auto& [name, value] : row.object) {
      const auto pos_it = name_to_idx.find(name);
      if (pos_it == name_to_idx.end()) continue;
      raw.features[pos_it->second].values[row_idx] = json_scalar_token(value);
    }
  }
  return raw;
}

CsvRawBatch raw_from_json_columns_object(const JsonValue& obj) {
  PLUSS_CHECK(obj.kind == JsonValue::Kind::Object, "loader::read_json_raw_batch: expected object root");
  CsvRawBatch raw;
  raw.features.reserve(obj.object.size());

  std::size_t max_rows = 0;
  for (std::size_t i = 0; i < obj.object.size(); ++i) {
    const auto& [name, value] = obj.object[i];
    std::vector<std::string> values;
    if (value.kind == JsonValue::Kind::Array) {
      values.reserve(value.array.size());
      for (const auto& cell : value.array) {
        values.push_back(json_scalar_token(cell));
      }
    } else {
      values.push_back(json_scalar_token(value));
    }
    max_rows = std::max(max_rows, values.size());
    raw.features.push_back(CsvRawColumn{name, i, std::move(values)});
  }

  for (auto& col : raw.features) {
    col.values.resize(max_rows);
  }

  return raw;
}

CsvRawBatch raw_from_json_split_object(const JsonValue& obj, const Config& config) {
  const auto* data = object_find(obj, "data");
  const auto* columns = object_find(obj, "columns");
  PLUSS_CHECK(data != nullptr && columns != nullptr,
              "loader::read_json_raw_batch: split JSON must contain 'data' and 'columns'");
  PLUSS_CHECK(data->kind == JsonValue::Kind::Array,
              "loader::read_json_raw_batch: split JSON 'data' must be an array");
  PLUSS_CHECK(columns->kind == JsonValue::Kind::Array,
              "loader::read_json_raw_batch: split JSON 'columns' must be an array");

  std::vector<std::string> names;
  names.reserve(columns->array.size());
  for (const auto& v : columns->array) {
    names.push_back(json_scalar_token(v));
  }
  if (names.empty()) {
    names = column_names_from_config(config);
  }

  CsvRawBatch raw;
  raw.features.reserve(names.size());
  for (std::size_t i = 0; i < names.size(); ++i) {
    raw.features.push_back(CsvRawColumn{
        names[i],
        i,
        std::vector<std::string>(data->array.size()),
    });
  }

  for (std::size_t row_idx = 0; row_idx < data->array.size(); ++row_idx) {
    const auto& row = data->array[row_idx];
    if (row.kind == JsonValue::Kind::Array) {
      const auto count = std::min<std::size_t>(row.array.size(), names.size());
      for (std::size_t col_idx = 0; col_idx < count; ++col_idx) {
        raw.features[col_idx].values[row_idx] = json_scalar_token(row.array[col_idx]);
      }
      continue;
    }
    if (row.kind == JsonValue::Kind::Object) {
      for (std::size_t col_idx = 0; col_idx < names.size(); ++col_idx) {
        if (const auto* v = object_find(row, names[col_idx]); v != nullptr) {
          raw.features[col_idx].values[row_idx] = json_scalar_token(*v);
        }
      }
      continue;
    }
    if (!raw.features.empty()) {
      raw.features[0].values[row_idx] = json_scalar_token(row);
    }
  }

  return raw;
}

CsvRawBatch raw_from_json_value(const JsonValue& root, const Config& config) {
  if (root.kind == JsonValue::Kind::Array) {
    if (root.array.empty()) return {};
    if (all_array_items_objects(root)) {
      return raw_from_json_records_array(root);
    }

    if (root.array.front().kind == JsonValue::Kind::Array) {
      const auto names_hint = column_names_from_config(config);
      std::size_t max_cols = names_hint.size();
      for (const auto& row : root.array) {
        if (row.kind == JsonValue::Kind::Array) {
          max_cols = std::max(max_cols, row.array.size());
        }
      }
      CsvRawBatch raw;
      raw.features.reserve(max_cols);
      for (std::size_t col = 0; col < max_cols; ++col) {
        const auto name = col < names_hint.size() ? names_hint[col] : fmt::format("feat_{}", col);
        raw.features.push_back(CsvRawColumn{
            name,
            col,
            std::vector<std::string>(root.array.size()),
        });
      }
      for (std::size_t row_idx = 0; row_idx < root.array.size(); ++row_idx) {
        const auto& row = root.array[row_idx];
        if (row.kind != JsonValue::Kind::Array) {
          raw.features[0].values[row_idx] = json_scalar_token(row);
          continue;
        }
        const auto count = std::min<std::size_t>(row.array.size(), raw.features.size());
        for (std::size_t col_idx = 0; col_idx < count; ++col_idx) {
          raw.features[col_idx].values[row_idx] = json_scalar_token(row.array[col_idx]);
        }
      }
      return raw;
    }

    CsvRawBatch raw;
    raw.features.push_back(CsvRawColumn{
        "value",
        0,
        std::vector<std::string>(root.array.size()),
    });
    for (std::size_t i = 0; i < root.array.size(); ++i) {
      raw.features[0].values[i] = json_scalar_token(root.array[i]);
    }
    return raw;
  }

  if (root.kind == JsonValue::Kind::Object) {
    if (const auto* records = object_find(root, "records");
        records != nullptr && records->kind == JsonValue::Kind::Array && all_array_items_objects(*records)) {
      return raw_from_json_records_array(*records);
    }
    if (object_find(root, "data") != nullptr && object_find(root, "columns") != nullptr) {
      return raw_from_json_split_object(root, config);
    }
    if (all_object_values_are_arrays_or_scalars(root)) {
      return raw_from_json_columns_object(root);
    }

    JsonValue as_records;
    as_records.kind = JsonValue::Kind::Array;
    as_records.array.push_back(root);
    return raw_from_json_records_array(as_records);
  }

  CsvRawBatch raw;
  raw.features.push_back(CsvRawColumn{
      "value",
      0,
      std::vector<std::string>{json_scalar_token(root)},
  });
  return raw;
}

void apply_projection_to_raw(CsvRawBatch& raw, const ReadCsvOptions& options) {
  if (options.ncases.has_value()) {
    const auto limit = *options.ncases;
    for (auto& col : raw.features) {
      if (col.values.size() > limit) col.values.resize(limit);
    }
    if (raw.cases.has_value() && raw.cases->values.size() > limit) {
      raw.cases->values.resize(limit);
    }
  }

  std::optional<std::size_t> case_col = std::nullopt;
  if (options.case_col_idx.has_value()) {
    const auto idx = *options.case_col_idx;
    PLUSS_CHECK(idx < raw.features.size(), "loader::read_json_raw_batch: case_col_idx out of range");
    case_col = idx;
  } else if (options.case_col.has_value()) {
    const auto it = std::find_if(raw.features.begin(),
                                 raw.features.end(),
                                 [&](const CsvRawColumn& col) { return col.name == *options.case_col; });
    PLUSS_CHECK(it != raw.features.end(),
                "loader::read_json_raw_batch: case column not found '" + *options.case_col + "'");
    case_col = static_cast<std::size_t>(std::distance(raw.features.begin(), it));
  }

  std::vector<std::size_t> selected_idx;
  if (!options.usefeats_idx.empty()) {
    selected_idx = options.usefeats_idx;
    for (const auto idx : selected_idx) {
      PLUSS_CHECK(idx < raw.features.size(), "loader::read_json_raw_batch: usefeats_idx out of range");
    }
  } else if (!options.usefeats.empty()) {
    selected_idx.reserve(options.usefeats.size());
    for (const auto& name : options.usefeats) {
      const auto it = std::find_if(raw.features.begin(),
                                   raw.features.end(),
                                   [&](const CsvRawColumn& col) { return col.name == name; });
      PLUSS_CHECK(it != raw.features.end(),
                  "loader::read_json_raw_batch: usefeats feature not found '" + name + "'");
      selected_idx.push_back(static_cast<std::size_t>(std::distance(raw.features.begin(), it)));
    }
  } else {
    selected_idx.resize(raw.features.size());
    for (std::size_t i = 0; i < raw.features.size(); ++i) selected_idx[i] = i;
  }

  if (case_col.has_value()) {
    raw.cases = std::move(raw.features[*case_col]);
    selected_idx.erase(std::remove(selected_idx.begin(), selected_idx.end(), *case_col), selected_idx.end());
  }

  if (selected_idx.size() != raw.features.size()) {
    std::vector<CsvRawColumn> projected;
    projected.reserve(selected_idx.size());
    for (const auto idx : selected_idx) {
      projected.push_back(std::move(raw.features[idx]));
    }
    raw.features = std::move(projected);
  }

  for (std::size_t i = 0; i < raw.features.size(); ++i) {
    raw.features[i].source_index = i;
  }
  if (raw.cases.has_value()) {
    raw.cases->source_index = raw.features.size();
  }
}

std::string read_text_file(std::string_view filepath, std::string_view where) {
  std::ifstream in(std::string(filepath), std::ios::in | std::ios::binary);
  PLUSS_CHECK(in.is_open(), std::string(where) + ": failed to open file '" + std::string(filepath) + "'");
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

WriteCsvOptions csv_write_options_from_config(const Config& config) {
  WriteCsvOptions options;

  if (const auto* v = find_config(config, {"sep", "delimiter", "separator"})) {
    if (auto s = parse_string_value(*v); s.has_value()) {
      options.sep = parse_sep_token(*s, options.sep);
    }
  }
  if (const auto* v = find_config(config, {"header"})) {
    if (auto b = parse_bool_value(*v); b.has_value()) {
      options.header = *b;
    }
  }
  if (const auto* v = find_config(config, {"addcases", "cases", "case", "caseindex", "index"})) {
    if (auto b = parse_bool_value(*v); b.has_value()) {
      options.add_cases = *b;
      options.cases = *b;
    }
  }
  if (const auto* v = find_config(config, {"caselabel", "indexlabel"})) {
    if (auto s = parse_string_value(*v); s.has_value()) {
      options.case_label = *s;
    }
  }
  if (const auto* v = find_config(config, {"narep", "na"})) {
    if (auto s = parse_string_value(*v); s.has_value()) {
      options.na_rep = *s;
    }
  }
  if (const auto* v = find_config(config, {"floatformat", "floatfmt"})) {
    if (auto s = parse_string_value(*v); s.has_value()) {
      options.float_format = *s;
    }
  }
  if (const auto* v = find_config(config, {"quotechar"})) {
    if (auto s = parse_string_value(*v); s.has_value() && !s->empty()) {
      options.quotechar = s->front();
    }
  }
  if (const auto* v = find_config(config, {"doublequote"})) {
    if (auto b = parse_bool_value(*v); b.has_value()) {
      options.doublequote = *b;
    }
  }
  if (const auto* v = find_config(config, {"lineterminator", "line_terminator"})) {
    if (auto s = parse_string_value(*v); s.has_value()) {
      options.line_terminator = *s;
    }
  }
  if (const auto* v = find_config(config, {"decimal"})) {
    if (auto s = parse_string_value(*v); s.has_value() && !s->empty()) {
      options.decimal = s->front();
    }
  }
  if (const auto* v = find_config(config, {"encoding"})) {
    if (auto s = parse_string_value(*v); s.has_value()) {
      options.encoding = *s;
    }
  }
  return options;
}

WriteExcelOptions excel_write_options_from_config(const Config& config) {
  WriteExcelOptions options;

  if (const auto* v = find_config(config, {"sheetname", "sheet"})) {
    if (auto s = parse_string_value(*v); s.has_value() && !s->empty()) {
      options.sheet_name = *s;
    }
  }
  if (const auto* v = find_config(config, {"addcases", "cases", "case", "caseindex", "index"})) {
    if (auto b = parse_bool_value(*v); b.has_value()) {
      options.add_cases = *b;
      options.cases = *b;
    }
  }
  if (const auto* v = find_config(config, {"header"})) {
    if (auto b = parse_bool_value(*v); b.has_value()) {
      options.header = *b;
    }
  }
  if (const auto* v = find_config(config, {"caselabel", "indexlabel"})) {
    if (auto s = parse_string_value(*v); s.has_value()) {
      options.case_label = *s;
    }
  }
  if (const auto* v = find_config(config, {"narep", "na"})) {
    if (auto s = parse_string_value(*v); s.has_value()) {
      options.na_rep = *s;
    }
  }
  return options;
}

#if PLUSSMIC_HAS_PARQUET_CPP
std::optional<pluss::Series> validity_mask_from_valid(const std::vector<bool>& valid) {
  if (valid.empty()) return std::nullopt;
  bool all_valid = true;
  for (const auto v : valid) all_valid = all_valid && v;
  if (all_valid) return std::nullopt;
  const auto n = static_cast<std::int64_t>(valid.size());
  auto mask = pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
  auto* p = reinterpret_cast<bool*>(mask.raw_data());
  for (std::int64_t i = 0; i < n; ++i) p[i] = valid[static_cast<std::size_t>(i)];
  return mask;
}

std::vector<bool> arrow_validity(const std::shared_ptr<arrow::Array>& array) {
  const auto n = static_cast<std::size_t>(array->length());
  std::vector<bool> valid(n, true);
  if (array->null_count() == 0) return valid;
  for (std::int64_t i = 0; i < array->length(); ++i) {
    if (array->IsNull(i)) valid[static_cast<std::size_t>(i)] = false;
  }
  return valid;
}

std::int64_t time_unit_to_nanos(arrow::TimeUnit::type unit) {
  switch (unit) {
    case arrow::TimeUnit::SECOND: return 1'000'000'000LL;
    case arrow::TimeUnit::MILLI: return 1'000'000LL;
    case arrow::TimeUnit::MICRO: return 1'000LL;
    case arrow::TimeUnit::NANO: return 1LL;
  }
  return 1LL;
}

pluss::table::Thread thread_from_arrow_array(const std::shared_ptr<arrow::Array>& array) {
  const auto valid = arrow_validity(array);
  const auto n = static_cast<std::int64_t>(array->length());
  const auto type_id = array->type_id();

  switch (type_id) {
    case arrow::Type::BOOL: {
      const auto a = std::static_pointer_cast<arrow::BooleanArray>(array);
      auto s = pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
      auto* p = reinterpret_cast<bool*>(s.raw_data());
      for (std::int64_t i = 0; i < n; ++i) p[i] = valid[static_cast<std::size_t>(i)] ? a->Value(i) : false;
      return pluss::table::Thread(s, validity_mask_from_valid(valid));
    }
    case arrow::Type::INT8:
    case arrow::Type::INT16:
    case arrow::Type::INT32:
    case arrow::Type::INT64:
    case arrow::Type::UINT8:
    case arrow::Type::UINT16:
    case arrow::Type::UINT32: {
      std::vector<std::int64_t> out(static_cast<std::size_t>(n), 0);
      for (std::int64_t i = 0; i < n; ++i) {
        if (!valid[static_cast<std::size_t>(i)]) continue;
        switch (type_id) {
          case arrow::Type::INT8:
            out[static_cast<std::size_t>(i)] = std::static_pointer_cast<arrow::Int8Array>(array)->Value(i);
            break;
          case arrow::Type::INT16:
            out[static_cast<std::size_t>(i)] = std::static_pointer_cast<arrow::Int16Array>(array)->Value(i);
            break;
          case arrow::Type::INT32:
            out[static_cast<std::size_t>(i)] = std::static_pointer_cast<arrow::Int32Array>(array)->Value(i);
            break;
          case arrow::Type::INT64:
            out[static_cast<std::size_t>(i)] = std::static_pointer_cast<arrow::Int64Array>(array)->Value(i);
            break;
          case arrow::Type::UINT8:
            out[static_cast<std::size_t>(i)] = std::static_pointer_cast<arrow::UInt8Array>(array)->Value(i);
            break;
          case arrow::Type::UINT16:
            out[static_cast<std::size_t>(i)] = std::static_pointer_cast<arrow::UInt16Array>(array)->Value(i);
            break;
          case arrow::Type::UINT32:
            out[static_cast<std::size_t>(i)] = std::static_pointer_cast<arrow::UInt32Array>(array)->Value(i);
            break;
          default:
            break;
        }
      }
      return pluss::table::Thread(pluss::make_series(out, {n}), validity_mask_from_valid(valid));
    }
    case arrow::Type::UINT64:
    case arrow::Type::HALF_FLOAT:
    case arrow::Type::FLOAT:
    case arrow::Type::DOUBLE: {
      std::vector<double> out(static_cast<std::size_t>(n), 0.0);
      for (std::int64_t i = 0; i < n; ++i) {
        if (!valid[static_cast<std::size_t>(i)]) continue;
        switch (type_id) {
          case arrow::Type::UINT64:
            out[static_cast<std::size_t>(i)] = static_cast<double>(std::static_pointer_cast<arrow::UInt64Array>(array)->Value(i));
            break;
          case arrow::Type::HALF_FLOAT:
            out[static_cast<std::size_t>(i)] = static_cast<double>(std::static_pointer_cast<arrow::HalfFloatArray>(array)->Value(i));
            break;
          case arrow::Type::FLOAT:
            out[static_cast<std::size_t>(i)] = static_cast<double>(std::static_pointer_cast<arrow::FloatArray>(array)->Value(i));
            break;
          case arrow::Type::DOUBLE:
            out[static_cast<std::size_t>(i)] = std::static_pointer_cast<arrow::DoubleArray>(array)->Value(i);
            break;
          default:
            break;
        }
      }
      return pluss::table::Thread(pluss::make_series(out, {n}), validity_mask_from_valid(valid));
    }
    case arrow::Type::DATE32:
    case arrow::Type::DATE64:
    case arrow::Type::TIMESTAMP: {
      std::vector<std::int64_t> ticks(static_cast<std::size_t>(n), 0);
      for (std::int64_t i = 0; i < n; ++i) {
        if (!valid[static_cast<std::size_t>(i)]) continue;
        if (type_id == arrow::Type::DATE32) {
          const auto days = std::static_pointer_cast<arrow::Date32Array>(array)->Value(i);
          ticks[static_cast<std::size_t>(i)] = static_cast<std::int64_t>(days) * 86'400'000'000'000LL;
        } else if (type_id == arrow::Type::DATE64) {
          const auto ms = std::static_pointer_cast<arrow::Date64Array>(array)->Value(i);
          ticks[static_cast<std::size_t>(i)] = static_cast<std::int64_t>(ms) * 1'000'000LL;
        } else {
          const auto* ts_type = static_cast<const arrow::TimestampType*>(array->type().get());
          const auto base = std::static_pointer_cast<arrow::TimestampArray>(array)->Value(i);
          ticks[static_cast<std::size_t>(i)] = base * time_unit_to_nanos(ts_type->unit());
        }
      }
      return pluss::table::Thread(pluss::from_epoch_nanos(ticks, {n}), validity_mask_from_valid(valid));
    }
    case arrow::Type::STRING:
    case arrow::Type::LARGE_STRING: {
      std::vector<std::string> out(static_cast<std::size_t>(n));
      for (std::int64_t i = 0; i < n; ++i) {
        if (!valid[static_cast<std::size_t>(i)]) continue;
        if (type_id == arrow::Type::STRING) {
          out[static_cast<std::size_t>(i)] = std::static_pointer_cast<arrow::StringArray>(array)->GetString(i);
        } else {
          out[static_cast<std::size_t>(i)] = std::static_pointer_cast<arrow::LargeStringArray>(array)->GetString(i);
        }
      }
      return pluss::table::Thread(pluss::make_note(out), validity_mask_from_valid(valid));
    }
    default: {
      std::vector<std::string> out(static_cast<std::size_t>(n));
      for (std::int64_t i = 0; i < n; ++i) {
        if (!valid[static_cast<std::size_t>(i)]) continue;
        auto scalar = array->GetScalar(i);
        if (scalar.ok() && *scalar) {
          out[static_cast<std::size_t>(i)] = (*scalar)->ToString();
        }
      }
      return pluss::table::Thread(pluss::make_note(out), validity_mask_from_valid(valid));
    }
  }
}

std::vector<std::string> raw_strings_from_arrow_array(const std::shared_ptr<arrow::Array>& array) {
  const auto n = static_cast<std::size_t>(array->length());
  std::vector<std::string> out(n);
  if (array->null_count() == array->length()) {
    return out;
  }

  const auto type_id = array->type_id();
  if (type_id == arrow::Type::STRING) {
    const auto a = std::static_pointer_cast<arrow::StringArray>(array);
    for (std::int64_t i = 0; i < array->length(); ++i) {
      if (a->IsNull(i)) continue;
      out[static_cast<std::size_t>(i)] = a->GetString(i);
    }
    return out;
  }
  if (type_id == arrow::Type::LARGE_STRING) {
    const auto a = std::static_pointer_cast<arrow::LargeStringArray>(array);
    for (std::int64_t i = 0; i < array->length(); ++i) {
      if (a->IsNull(i)) continue;
      out[static_cast<std::size_t>(i)] = a->GetString(i);
    }
    return out;
  }

  for (std::int64_t i = 0; i < array->length(); ++i) {
    if (array->IsNull(i)) continue;
    auto scalar = array->GetScalar(i);
    if (scalar.ok() && *scalar) {
      out[static_cast<std::size_t>(i)] = (*scalar)->ToString();
    }
  }
  return out;
}

std::vector<bool> thread_validity(const pluss::table::Thread& thread) {
  const auto n = static_cast<std::size_t>(thread.size());
  if (!thread.has_validity_mask()) {
    return std::vector<bool>(n, true);
  }
  auto valid = thread.validity_mask()->to_vec<bool>();
  PLUSS_CHECK(valid.size() == n, "loader::save_parquet: thread validity mask size mismatch");
  return valid;
}

std::shared_ptr<arrow::Array> note_thread_to_arrow_string_array(const pluss::table::Thread& thread) {
  const auto valid = thread_validity(thread);
  const auto note = thread.as_note();
  PLUSS_CHECK(static_cast<std::size_t>(note.numel()) == valid.size(),
              "loader::save_parquet: Note thread length mismatch");
  arrow::StringBuilder builder;
  for (std::int64_t i = 0; i < note.numel(); ++i) {
    const auto idx = static_cast<std::size_t>(i);
    auto st = valid[idx] ? builder.Append(std::string(note.at(i))) : builder.AppendNull();
    PLUSS_CHECK(st.ok(), "loader::save_parquet: failed to append Note value: " + st.ToString());
  }
  std::shared_ptr<arrow::Array> out;
  auto st = builder.Finish(&out);
  PLUSS_CHECK(st.ok() && out != nullptr, "loader::save_parquet: failed to finalize Note arrow array: " + st.ToString());
  return out;
}

std::shared_ptr<arrow::Array> series_thread_to_arrow_array(const pluss::table::Thread& thread) {
  const auto valid = thread_validity(thread);
  const auto series = thread.as_series();
  const auto n = static_cast<std::size_t>(series.numel());
  PLUSS_CHECK(n == valid.size(), "loader::save_parquet: Series thread length mismatch");

  auto finish_or_throw = [](auto& builder, std::string_view what) -> std::shared_ptr<arrow::Array> {
    std::shared_ptr<arrow::Array> out;
    auto st = builder.Finish(&out);
    PLUSS_CHECK(st.ok() && out != nullptr,
                "loader::save_parquet: failed to finalize " + std::string(what) + " arrow array: " + st.ToString());
    return out;
  };

  switch (series.scalar_type()) {
    case pluss::codem::ScalarType::Bool: {
      const auto values = series.to_vec<bool>();
      arrow::BooleanBuilder builder;
      for (std::size_t i = 0; i < n; ++i) {
        auto st = valid[i] ? builder.Append(values[i]) : builder.AppendNull();
        PLUSS_CHECK(st.ok(), "loader::save_parquet: failed to append Bool value: " + st.ToString());
      }
      return finish_or_throw(builder, "Bool");
    }
    case pluss::codem::ScalarType::Int8: {
      const auto values = series.to_vec<std::int8_t>();
      arrow::Int8Builder builder;
      for (std::size_t i = 0; i < n; ++i) {
        auto st = valid[i] ? builder.Append(values[i]) : builder.AppendNull();
        PLUSS_CHECK(st.ok(), "loader::save_parquet: failed to append Int8 value: " + st.ToString());
      }
      return finish_or_throw(builder, "Int8");
    }
    case pluss::codem::ScalarType::Int16: {
      const auto values = series.to_vec<std::int16_t>();
      arrow::Int16Builder builder;
      for (std::size_t i = 0; i < n; ++i) {
        auto st = valid[i] ? builder.Append(values[i]) : builder.AppendNull();
        PLUSS_CHECK(st.ok(), "loader::save_parquet: failed to append Int16 value: " + st.ToString());
      }
      return finish_or_throw(builder, "Int16");
    }
    case pluss::codem::ScalarType::Int32: {
      const auto values = series.to_vec<std::int32_t>();
      arrow::Int32Builder builder;
      for (std::size_t i = 0; i < n; ++i) {
        auto st = valid[i] ? builder.Append(values[i]) : builder.AppendNull();
        PLUSS_CHECK(st.ok(), "loader::save_parquet: failed to append Int32 value: " + st.ToString());
      }
      return finish_or_throw(builder, "Int32");
    }
    case pluss::codem::ScalarType::Int64: {
      const auto values = series.to_vec<std::int64_t>();
      arrow::Int64Builder builder;
      for (std::size_t i = 0; i < n; ++i) {
        auto st = valid[i] ? builder.Append(values[i]) : builder.AppendNull();
        PLUSS_CHECK(st.ok(), "loader::save_parquet: failed to append Int64 value: " + st.ToString());
      }
      return finish_or_throw(builder, "Int64");
    }
    case pluss::codem::ScalarType::UInt8: {
      const auto values = series.to_vec<std::uint8_t>();
      arrow::UInt8Builder builder;
      for (std::size_t i = 0; i < n; ++i) {
        auto st = valid[i] ? builder.Append(values[i]) : builder.AppendNull();
        PLUSS_CHECK(st.ok(), "loader::save_parquet: failed to append UInt8 value: " + st.ToString());
      }
      return finish_or_throw(builder, "UInt8");
    }
    case pluss::codem::ScalarType::UInt16: {
      const auto values = series.to_vec<std::uint16_t>();
      arrow::UInt16Builder builder;
      for (std::size_t i = 0; i < n; ++i) {
        auto st = valid[i] ? builder.Append(values[i]) : builder.AppendNull();
        PLUSS_CHECK(st.ok(), "loader::save_parquet: failed to append UInt16 value: " + st.ToString());
      }
      return finish_or_throw(builder, "UInt16");
    }
    case pluss::codem::ScalarType::UInt32: {
      const auto values = series.to_vec<std::uint32_t>();
      arrow::UInt32Builder builder;
      for (std::size_t i = 0; i < n; ++i) {
        auto st = valid[i] ? builder.Append(values[i]) : builder.AppendNull();
        PLUSS_CHECK(st.ok(), "loader::save_parquet: failed to append UInt32 value: " + st.ToString());
      }
      return finish_or_throw(builder, "UInt32");
    }
    case pluss::codem::ScalarType::UInt64: {
      const auto values = series.to_vec<std::uint64_t>();
      arrow::UInt64Builder builder;
      for (std::size_t i = 0; i < n; ++i) {
        auto st = valid[i] ? builder.Append(values[i]) : builder.AppendNull();
        PLUSS_CHECK(st.ok(), "loader::save_parquet: failed to append UInt64 value: " + st.ToString());
      }
      return finish_or_throw(builder, "UInt64");
    }
    case pluss::codem::ScalarType::Float16:
    case pluss::codem::ScalarType::BFloat16:
    case pluss::codem::ScalarType::Float32: {
      const auto values = series.to(pluss::codem::ScalarType::Float32).to_vec<float>();
      arrow::FloatBuilder builder;
      for (std::size_t i = 0; i < n; ++i) {
        auto st = valid[i] ? builder.Append(values[i]) : builder.AppendNull();
        PLUSS_CHECK(st.ok(), "loader::save_parquet: failed to append Float32 value: " + st.ToString());
      }
      return finish_or_throw(builder, "Float32");
    }
    case pluss::codem::ScalarType::Float64: {
      const auto values = series.to_vec<double>();
      arrow::DoubleBuilder builder;
      for (std::size_t i = 0; i < n; ++i) {
        auto st = valid[i] ? builder.Append(values[i]) : builder.AppendNull();
        PLUSS_CHECK(st.ok(), "loader::save_parquet: failed to append Float64 value: " + st.ToString());
      }
      return finish_or_throw(builder, "Float64");
    }
    default:
      return note_thread_to_arrow_string_array(thread.cast(pluss::table::ThreadCastTarget::Note));
  }
}

std::shared_ptr<arrow::Array> timeline_thread_to_arrow_array(const pluss::table::Thread& thread) {
  const auto valid = thread_validity(thread);
  const auto ticks = thread.as_timeline().to_epoch_nanos_vector();
  PLUSS_CHECK(ticks.size() == valid.size(), "loader::save_parquet: Timeline thread length mismatch");
  arrow::TimestampBuilder builder(arrow::timestamp(arrow::TimeUnit::NANO), arrow::default_memory_pool());
  for (std::size_t i = 0; i < ticks.size(); ++i) {
    auto st = valid[i] ? builder.Append(ticks[i]) : builder.AppendNull();
    PLUSS_CHECK(st.ok(), "loader::save_parquet: failed to append Timeline value: " + st.ToString());
  }
  std::shared_ptr<arrow::Array> out;
  auto st = builder.Finish(&out);
  PLUSS_CHECK(st.ok() && out != nullptr, "loader::save_parquet: failed to finalize Timeline arrow array: " + st.ToString());
  return out;
}

std::shared_ptr<arrow::Array> timeindex_thread_to_arrow_array(const pluss::table::Thread& thread) {
  const auto valid = thread_validity(thread);
  const auto ticks = thread.as_timeindex().timeline().to_epoch_nanos_vector();
  PLUSS_CHECK(ticks.size() == valid.size(), "loader::save_parquet: TimeIndex thread length mismatch");
  arrow::TimestampBuilder builder(arrow::timestamp(arrow::TimeUnit::NANO), arrow::default_memory_pool());
  for (std::size_t i = 0; i < ticks.size(); ++i) {
    auto st = valid[i] ? builder.Append(ticks[i]) : builder.AppendNull();
    PLUSS_CHECK(st.ok(), "loader::save_parquet: failed to append TimeIndex value: " + st.ToString());
  }
  std::shared_ptr<arrow::Array> out;
  auto st = builder.Finish(&out);
  PLUSS_CHECK(st.ok() && out != nullptr, "loader::save_parquet: failed to finalize TimeIndex arrow array: " + st.ToString());
  return out;
}

std::shared_ptr<arrow::Array> thread_to_arrow_array(const pluss::table::Thread& thread) {
  switch (thread.kind()) {
    case pluss::table::ThreadKind::Series:
      return series_thread_to_arrow_array(thread);
    case pluss::table::ThreadKind::Note:
      return note_thread_to_arrow_string_array(thread);
    case pluss::table::ThreadKind::Timeline:
      return timeline_thread_to_arrow_array(thread);
    case pluss::table::ThreadKind::TimeIndex:
      return timeindex_thread_to_arrow_array(thread);
    default:
      return note_thread_to_arrow_string_array(thread.cast(pluss::table::ThreadCastTarget::Note));
  }
}

parquet::Compression::type parquet_compression_from_config(const Config& config) {
  if (const auto* v = find_config(config, {"compression", "parquetcompression", "codec"})) {
    if (auto s = parse_string_value(*v); s.has_value()) {
      const auto low = normalize_key(*s);
      if (low == "none" || low == "off" || low == "uncompressed") {
        return parquet::Compression::UNCOMPRESSED;
      }
      if (low == "gzip") return parquet::Compression::GZIP;
      if (low == "brotli") return parquet::Compression::BROTLI;
      if (low == "zstd") return parquet::Compression::ZSTD;
      if (low == "lz4") return parquet::Compression::LZ4;
      return parquet::Compression::SNAPPY;
    }
  }
  return parquet::Compression::SNAPPY;
}

void save_parquet_cpp(const pluss::table::DataFrame& frame,
                      std::string_view filepath,
                      const Config& config) {
  PLUSS_CHECK(frame.defined(), "loader::save_parquet: frame is undefined");
  PLUSS_CHECK(!trim_ascii(filepath).empty(), "loader::save_parquet: filepath cannot be empty");

  bool add_cases = false;
  if (const auto* v = find_config(config, {"addcases", "cases", "case", "index"})) {
    if (auto b = parse_bool_value(*v); b.has_value()) {
      add_cases = *b;
    }
  }

  std::string case_label = "case";
  if (const auto* v = find_config(config, {"caselabel", "indexlabel", "case_feature"})) {
    if (auto s = parse_string_value(*v); s.has_value() && !s->empty()) {
      case_label = *s;
    }
  }

  auto names = frame.feature_names();
  auto columns = frame.features();
  if (add_cases) {
    names.insert(names.begin(), case_label);
    columns.insert(columns.begin(), frame.cases());
  }
  PLUSS_CHECK(names.size() == columns.size(), "loader::save_parquet: column metadata size mismatch");

  std::unordered_map<std::string, std::size_t> seen;
  seen.reserve(names.size());
  std::vector<std::shared_ptr<arrow::Field>> fields;
  std::vector<std::shared_ptr<arrow::Array>> arrays;
  fields.reserve(names.size());
  arrays.reserve(names.size());

  for (std::size_t i = 0; i < columns.size(); ++i) {
    auto name = trim_ascii(names[i]);
    if (name.empty()) {
      name = fmt::format("feat_{}", i);
    }
    auto [it, inserted] = seen.emplace(name, 1);
    if (!inserted) {
      const auto nth = ++(it->second);
      name = fmt::format("{}_{}", name, nth);
    }

    auto array = thread_to_arrow_array(columns[i]);
    PLUSS_CHECK(array != nullptr, "loader::save_parquet: failed to build arrow array for column '" + name + "'");
    fields.emplace_back(arrow::field(name, array->type(), array->null_count() > 0));
    arrays.emplace_back(std::move(array));
  }

  const auto schema = arrow::schema(fields);
  auto table = arrow::Table::Make(schema, arrays, frame.ncases());
  PLUSS_CHECK(table != nullptr, "loader::save_parquet: failed to assemble arrow table");

  auto maybe_out = arrow::io::FileOutputStream::Open(std::string(filepath));
  PLUSS_CHECK(maybe_out.ok(),
              "loader::save_parquet: unable to open output file '" + std::string(filepath) + "': " +
                  maybe_out.status().ToString());
  auto outfile = *maybe_out;

  parquet::WriterProperties::Builder writer_builder;
  writer_builder.compression(parquet_compression_from_config(config));
  if (const auto* v = find_config(config, {"usedictionary", "dictionary", "parquetdictionary"})) {
    if (auto b = parse_bool_value(*v); b.has_value()) {
      if (*b) {
        writer_builder.enable_dictionary();
      } else {
        writer_builder.disable_dictionary();
      }
    }
  }
  auto writer_props = writer_builder.build();

  std::int64_t row_group_size = std::max<std::int64_t>(1, frame.ncases());
  if (const auto* v = find_config(config, {"rowgroupsize", "chunksize", "parquetchunksize"})) {
    if (auto n = parse_int_value(*v); n.has_value() && *n > 0) {
      row_group_size = *n;
    }
  }

  auto st = parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), outfile, row_group_size, writer_props);
  PLUSS_CHECK(st.ok(), "loader::save_parquet: parquet write failed: " + st.ToString());
  auto close_status = outfile->Close();
  PLUSS_CHECK(close_status.ok(), "loader::save_parquet: failed to finalize output file: " + close_status.ToString());
}

CsvRawBatch read_parquet_raw_batch_cpp(std::string_view filepath, const Config& config) {
  auto maybe_file = arrow::io::ReadableFile::Open(std::string(filepath));
  PLUSS_CHECK(maybe_file.ok(), "loader::read_parquet_raw_batch: unable to open file '" + std::string(filepath) + "'");
  auto infile = *maybe_file;

  std::unique_ptr<parquet::arrow::FileReader> reader;
  auto st = parquet::arrow::OpenFile(infile, arrow::default_memory_pool(), &reader);
  PLUSS_CHECK(st.ok(), "loader::read_parquet_raw_batch: failed to open parquet reader: " + st.ToString());

  std::shared_ptr<arrow::Schema> schema;
  st = reader->GetSchema(&schema);
  PLUSS_CHECK(st.ok() && schema != nullptr, "loader::read_parquet_raw_batch: failed to read schema: " + st.ToString());

  std::vector<int> column_indices;
  if (const auto* cols_cfg = find_config(config, {"usefeats", "features", "selectfeats"})) {
    if (auto feats = parse_string_list_value(*cols_cfg); feats.has_value() && !feats->empty()) {
      column_indices.reserve(feats->size());
      for (const auto& col : *feats) {
        const auto idx = schema->GetFieldIndex(col);
        PLUSS_CHECK(idx >= 0, "loader::read_parquet_raw_batch: requested column not found '" + col + "'");
        column_indices.push_back(idx);
      }
    }
  }

  std::shared_ptr<arrow::Table> table;
  if (column_indices.empty()) {
    st = reader->ReadTable(&table);
  } else {
    st = reader->ReadTable(column_indices, &table);
  }
  PLUSS_CHECK(st.ok() && table != nullptr, "loader::read_parquet_raw_batch: failed to read table: " + st.ToString());

  if (const auto* n_cfg = find_config(config, {"ncases"})) {
    if (auto n = parse_int_value(*n_cfg); n.has_value() && *n >= 0) {
      table = table->Slice(0, *n);
    }
  }

  auto combined = table->CombineChunks(arrow::default_memory_pool());
  if (combined.ok()) {
    table = *combined;
  }

  CsvRawBatch raw;
  raw.features.reserve(static_cast<std::size_t>(table->num_columns()));
  for (int i = 0; i < table->num_columns(); ++i) {
    const auto& col = table->column(i);
    PLUSS_CHECK(col != nullptr, "loader::read_parquet_raw_batch: encountered null column handle");
    std::shared_ptr<arrow::Array> array;
    if (col->num_chunks() == 0) {
      array = arrow::MakeArrayOfNull(col->type(), table->num_rows()).ValueOrDie();
    } else {
      array = col->chunk(0);
    }
    const auto source_idx = column_indices.empty()
        ? static_cast<std::size_t>(i)
        : static_cast<std::size_t>(column_indices[static_cast<std::size_t>(i)]);
    raw.features.push_back(CsvRawColumn{
        table->field(i)->name(),
        source_idx,
        raw_strings_from_arrow_array(array),
    });
  }

  if (const auto* v = find_config(config, {"casecol", "case", "caseindex"})) {
    if (auto idx = parse_int_value(*v); idx.has_value()) {
      PLUSS_CHECK(*idx >= 0 && *idx < static_cast<std::int64_t>(raw.features.size()),
                  "loader::read_parquet_raw_batch: case index out of bounds");
      const auto case_pos = static_cast<std::size_t>(*idx);
      raw.cases = std::move(raw.features[case_pos]);
      raw.features.erase(raw.features.begin() + static_cast<std::ptrdiff_t>(case_pos));
    } else if (auto name = parse_string_value(*v); name.has_value() && !is_none_token(*name)) {
      const auto it = std::find_if(raw.features.begin(),
                                   raw.features.end(),
                                   [&](const CsvRawColumn& col) { return col.name == *name; });
      PLUSS_CHECK(it != raw.features.end(), "loader::read_parquet_raw_batch: case column not found '" + *name + "'");
      raw.cases = std::move(*it);
      raw.features.erase(it);
    }
  }

  return raw;
}

pluss::table::DataFrame load_parquet_cpp(std::string_view filepath, const Config& config) {
  auto maybe_file = arrow::io::ReadableFile::Open(std::string(filepath));
  PLUSS_CHECK(maybe_file.ok(), "loader::load_parquet: unable to open file '" + std::string(filepath) + "'");
  auto infile = *maybe_file;

  std::unique_ptr<parquet::arrow::FileReader> reader;
  auto st = parquet::arrow::OpenFile(infile, arrow::default_memory_pool(), &reader);
  PLUSS_CHECK(st.ok(), "loader::load_parquet: failed to open parquet reader: " + st.ToString());

  std::shared_ptr<arrow::Schema> schema;
  st = reader->GetSchema(&schema);
  PLUSS_CHECK(st.ok() && schema != nullptr, "loader::load_parquet: failed to read schema: " + st.ToString());

  std::vector<int> column_indices;
  if (const auto* cols_cfg = find_config(config, {"usefeats", "features", "selectfeats"})) {
    if (auto feats = parse_string_list_value(*cols_cfg); feats.has_value() && !feats->empty()) {
      column_indices.reserve(feats->size());
      for (const auto& col : *feats) {
        const auto idx = schema->GetFieldIndex(col);
        PLUSS_CHECK(idx >= 0, "loader::load_parquet: requested column not found '" + col + "'");
        column_indices.push_back(idx);
      }
    }
  }

  std::shared_ptr<arrow::Table> table;
  if (column_indices.empty()) {
    st = reader->ReadTable(&table);
  } else {
    st = reader->ReadTable(column_indices, &table);
  }
  PLUSS_CHECK(st.ok() && table != nullptr, "loader::load_parquet: failed to read table: " + st.ToString());

  if (const auto* n_cfg = find_config(config, {"ncases"})) {
    if (auto n = parse_int_value(*n_cfg); n.has_value() && *n >= 0) {
      table = table->Slice(0, *n);
    }
  }

  auto combined = table->CombineChunks(arrow::default_memory_pool());
  if (combined.ok()) {
    table = *combined;
  }

  std::vector<std::pair<std::string, pluss::table::Thread>> features;
  features.reserve(static_cast<std::size_t>(table->num_columns()));
  for (int i = 0; i < table->num_columns(); ++i) {
    const auto& col = table->column(i);
    PLUSS_CHECK(col != nullptr, "loader::load_parquet: encountered null column handle");
    std::shared_ptr<arrow::Array> array;
    if (col->num_chunks() == 0) {
      array = arrow::MakeArrayOfNull(col->type(), table->num_rows()).ValueOrDie();
    } else {
      array = col->chunk(0);
    }
    features.emplace_back(table->field(i)->name(), thread_from_arrow_array(array));
  }

  std::optional<pluss::table::Thread> cases = std::nullopt;
  if (const auto* v = find_config(config, {"casecol", "case", "caseindex"})) {
    if (auto idx = parse_int_value(*v); idx.has_value()) {
      PLUSS_CHECK(*idx >= 0 && *idx < static_cast<std::int64_t>(features.size()),
                  "loader::load_parquet: case index out of bounds");
      const auto case_pos = static_cast<std::size_t>(*idx);
      cases = features[case_pos].second;
      features.erase(features.begin() + static_cast<std::ptrdiff_t>(case_pos));
    } else if (auto name = parse_string_value(*v); name.has_value() && !is_none_token(*name)) {
      const auto it = std::find_if(features.begin(), features.end(), [&](const auto& kv) { return kv.first == *name; });
      PLUSS_CHECK(it != features.end(), "loader::load_parquet: case column not found '" + *name + "'");
      cases = it->second;
      features.erase(it);
    }
  }

  return pluss::table::DataFrame(std::move(features), std::move(cases));
}
#endif

} // namespace

ReadCsvOptions csv_options_from_config(const Config& config) {
  return csv_read_options_from_config(config);
}

ReadExcelOptions excel_options_from_config(const Config& config) {
  return excel_read_options_from_config(config);
}

SheetNames::SheetNames(std::vector<std::string> names) : names_(std::move(names)) {}

const std::vector<std::string>& SheetNames::values() const noexcept { return names_; }
std::size_t SheetNames::size() const noexcept { return names_.size(); }
bool SheetNames::empty() const noexcept { return names_.empty(); }

std::ostream& operator<<(std::ostream& os, const SheetNames& names) {
  os << "[";
  for (std::size_t i = 0; i < names.values().size(); ++i) {
    if (i) {
      os << ", ";
    }
    os << names.values()[i];
  }
  os << "]";
  return os;
}

LoadResult::LoadResult(pluss::table::DataFrame frame)
    : is_workbook_(false), frame_(std::move(frame)) {}

LoadResult::LoadResult(Workbook workbook)
    : is_workbook_(true), workbook_(std::move(workbook)) {}

bool LoadResult::is_frame() const noexcept { return !is_workbook_; }
bool LoadResult::is_workbook() const noexcept { return is_workbook_; }

const pluss::table::DataFrame& LoadResult::dataframe() const {
  PLUSS_CHECK(!is_workbook_, "LoadResult::dataframe: result is a workbook, not a DataFrame");
  return frame_;
}

pluss::table::DataFrame& LoadResult::dataframe() {
  PLUSS_CHECK(!is_workbook_, "LoadResult::dataframe: result is a workbook, not a DataFrame");
  return frame_;
}

const Workbook& LoadResult::workbook() const {
  PLUSS_CHECK(is_workbook_, "LoadResult::workbook: result is a DataFrame, not a workbook");
  return workbook_;
}

Workbook& LoadResult::workbook() {
  PLUSS_CHECK(is_workbook_, "LoadResult::workbook: result is a DataFrame, not a workbook");
  return workbook_;
}

const pluss::table::DataFrame& LoadResult::sheets(std::string_view name) const {
  const auto& wb = workbook();
  const auto key = normalize_key(std::string(name));
  for (const auto& [sheet_name, frame] : wb) {
    if (normalize_key(sheet_name) == key) {
      return frame;
    }
  }
  PLUSS_FAIL("LoadResult::sheets: sheet '" + std::string(name) + "' not found");
}

pluss::table::DataFrame& LoadResult::sheets(std::string_view name) {
  auto& wb = workbook();
  const auto key = normalize_key(std::string(name));
  for (auto& [sheet_name, frame] : wb) {
    if (normalize_key(sheet_name) == key) {
      return frame;
    }
  }
  PLUSS_FAIL("LoadResult::sheets: sheet '" + std::string(name) + "' not found");
}

const pluss::table::DataFrame& LoadResult::sheet_by_index(std::size_t index) const {
  const auto& wb = workbook();
  PLUSS_CHECK(index < wb.size(), "LoadResult::sheet_by_index: index out of range");
  return wb[index].second;
}

pluss::table::DataFrame& LoadResult::sheet_by_index(std::size_t index) {
  auto& wb = workbook();
  PLUSS_CHECK(index < wb.size(), "LoadResult::sheet_by_index: index out of range");
  return wb[index].second;
}

SheetNames LoadResult::sheet_names() const {
  std::vector<std::string> names;
  if (is_workbook_) {
    names.reserve(workbook_.size());
    for (const auto& [name, _] : workbook_) {
      (void)_;
      names.push_back(name);
    }
  }
  return SheetNames(std::move(names));
}

std::ostream& operator<<(std::ostream& os, const LoadResult& loaded) {
  if (loaded.is_frame()) {
    os << loaded.dataframe();
    return os;
  }

  const auto& wb = loaded.workbook();
  os << fmt::format("Workbook({} sheets)", wb.size());
  for (std::size_t i = 0; i < wb.size(); ++i) {
    const auto& [name, frame] = wb[i];
    os << fmt::format("\n  [{}] {} -> [{} cs x {} feat]", i, name, frame.ncases(), frame.nfeats());
  }
  return os;
}

CsvRawBatch read_csv_raw_batch(std::string_view filepath, const Config& config) {
  return ::pluss::loader::read_csv_raw_batch(filepath, csv_read_options_from_config(config));
}

CsvRawBatch read_csv_raw_batch(std::string_view filepath,
                               std::initializer_list<ConfigEntry> config_entries) {
  return read_csv_raw_batch(filepath, config_from(config_entries));
}

CsvRawBatch read_txt_raw_batch(std::string_view filepath, const Config& config) {
  auto source = detail::resolve_local_or_http_source(filepath, "loader::read_txt_raw_batch");
  const std::string_view local_filepath(source.local_path());

  const auto mode = txt_parse_mode_from_config(config);
  auto csv_options = csv_read_options_from_config(config);
  csv_options.filepath = source.local_path();

  if (mode == TxtParseMode::Delimited) {
    return ::pluss::loader::read_csv_raw_batch(csv_options);
  }

  const auto widths = parse_widths_config(config);
  const auto min_gap = mode == TxtParseMode::Whitespace ? std::size_t{1} : std::size_t{2};
  const auto records = read_text_records_with_mode(local_filepath, min_gap, widths, csv_options.skip_blank_lines);
  if (mode == TxtParseMode::Whitespace || mode == TxtParseMode::FixedWidth) {
    return parse_txt_records_as_raw_batch(records, csv_options);
  }

  // Auto mode: try CSV-style separator detection first, then whitespace/fixed-width fallback.
  auto csv_raw = ::pluss::loader::read_csv_raw_batch(csv_options);
  if (csv_raw.features.size() > 1) {
    return csv_raw;
  }

  auto whitespace_records = read_text_records_with_mode(local_filepath, 1, {}, csv_options.skip_blank_lines);
  auto fixed_gap_records = read_text_records_with_mode(local_filepath, 2, widths, csv_options.skip_blank_lines);
  const auto whitespace_score = score_record_grid(whitespace_records);
  const auto fixed_score = score_record_grid(fixed_gap_records);

  if (whitespace_score <= 0.0 && fixed_score <= 0.0) {
    return csv_raw;
  }

  const auto& selected = whitespace_score >= fixed_score ? whitespace_records : fixed_gap_records;
  return parse_txt_records_as_raw_batch(selected, csv_options);
}

CsvRawBatch read_txt_raw_batch(std::string_view filepath,
                               std::initializer_list<ConfigEntry> config_entries) {
  return read_txt_raw_batch(filepath, config_from(config_entries));
}

CsvRawBatch read_excel_raw_batch(std::string_view filepath, const Config& config) {
  return ::pluss::loader::read_excel_raw_batch(filepath, excel_read_options_from_config(config));
}

CsvRawBatch read_excel_raw_batch(std::string_view filepath,
                                 std::initializer_list<ConfigEntry> config_entries) {
  return read_excel_raw_batch(filepath, config_from(config_entries));
}

ExcelRawBatchWorkbook read_excel_workbook_raw_batch(std::string_view filepath, const Config& config) {
  auto opts = excel_read_options_from_config(config);
  if (!opts.all_sheets && opts.sheet_names.empty() && opts.sheet_indices.empty() && !opts.n_sheets.has_value()) {
    opts.all_sheets = true;
  }
  return ::pluss::loader::read_excel_workbook_raw_batch(filepath, opts);
}

ExcelRawBatchWorkbook read_excel_workbook_raw_batch(std::string_view filepath,
                                                    std::initializer_list<ConfigEntry> config_entries) {
  return read_excel_workbook_raw_batch(filepath, config_from(config_entries));
}

CsvBatch read_csv_batch(std::string_view filepath, const Config& config) {
  return ::pluss::loader::read_csv_batch(filepath, csv_read_options_from_config(config));
}

CsvBatch read_csv_batch(std::string_view filepath,
                        std::initializer_list<ConfigEntry> config_entries) {
  return read_csv_batch(filepath, config_from(config_entries));
}

CsvBatch read_excel_batch(std::string_view filepath, const Config& config) {
  return ::pluss::loader::read_excel_batch(filepath, excel_read_options_from_config(config));
}

CsvBatch read_excel_batch(std::string_view filepath,
                          std::initializer_list<ConfigEntry> config_entries) {
  return read_excel_batch(filepath, config_from(config_entries));
}

ExcelBatchWorkbook read_excel_workbook_batch(std::string_view filepath, const Config& config) {
  auto opts = excel_read_options_from_config(config);
  if (!opts.all_sheets && opts.sheet_names.empty() && opts.sheet_indices.empty() && !opts.n_sheets.has_value()) {
    opts.all_sheets = true;
  }
  return ::pluss::loader::read_excel_workbook_batch(filepath, opts);
}

ExcelBatchWorkbook read_excel_workbook_batch(std::string_view filepath,
                                             std::initializer_list<ConfigEntry> config_entries) {
  return read_excel_workbook_batch(filepath, config_from(config_entries));
}

pluss::table::DataFrame load_csv(std::string_view filepath, const Config& config) {
  auto batch = read_csv_batch(filepath, config);
  return pluss::table::DataFrame(std::move(batch.features), std::move(batch.cases));
}

pluss::table::DataFrame load_csv(std::string_view filepath,
                                 std::initializer_list<ConfigEntry> config_entries) {
  return load_csv(filepath, config_from(config_entries));
}

pluss::table::DataFrame load_csv(std::string_view filepath,
                                 std::string_view header,
                                 std::string_view case_feature,
                                 std::string_view engine,
                                 std::optional<std::size_t> ncases) {
  ReadCsvOptions options;
  options.header = !is_none_token(header);
  if (!is_none_token(case_feature)) {
    options.case_col = std::string(case_feature);
  }
  if (ncases.has_value()) {
    options.ncases = *ncases;
  }

  const auto eng = normalize_key(std::string(engine));
  if (eng == "parallel") {
    options.engine = CsvEngine::Parallel;
  } else if (eng == "serial") {
    options.engine = CsvEngine::Serial;
  } else {
    options.engine = CsvEngine::Auto;
  }

  return ::pluss::loader::load_csv(filepath, options);
}

pluss::table::DataFrame load_csv(std::string_view filepath,
                                 std::string_view header,
                                 std::size_t ncases) {
  return load_csv(filepath, header, "none", "auto", ncases);
}

pluss::table::DataFrame load_txt(std::string_view filepath, const Config& config) {
  const auto options = csv_read_options_from_config(config);
  return dataframe_from_raw_batch_padded(read_txt_raw_batch(filepath, config), options);
}

pluss::table::DataFrame load_txt(std::string_view filepath,
                                 std::initializer_list<ConfigEntry> config_entries) {
  return load_txt(filepath, config_from(config_entries));
}

pluss::table::DataFrame load_excel(std::string_view filepath, const Config& config) {
  auto batch = read_excel_batch(filepath, config);
  return pluss::table::DataFrame(std::move(batch.features), std::move(batch.cases));
}

pluss::table::DataFrame load_excel(std::string_view filepath,
                                   std::initializer_list<ConfigEntry> config_entries) {
  return load_excel(filepath, config_from(config_entries));
}

pluss::table::DataFrame load_excel(std::string_view filepath,
                                   std::string_view sheet_name,
                                   bool header) {
  PLUSS_CHECK(normalize_key(std::string(sheet_name)) != "none",
              "loader::load_excel(sheet_name=\"none\") is workbook mode; use loader::load_excel(filepath, nullptr, header)");
  ReadExcelOptions options;
  options.header = header;
  options.sheet_name = std::string(sheet_name);
  return ::pluss::loader::load_excel(filepath, options);
}

pluss::table::DataFrame load_excel(std::string_view filepath,
                                   std::int64_t sheet_index,
                                   bool header) {
  ReadExcelOptions options;
  options.header = header;
  options.sheet_index = sheet_index;
  return ::pluss::loader::load_excel(filepath, options);
}

LoadResult load_excel(std::string_view filepath, std::nullptr_t, bool header) {
  ReadExcelOptions options;
  options.header = header;
  options.all_sheets = true;
  auto wb_batch = ::pluss::loader::read_excel_workbook_batch(filepath, options);
  Workbook wb;
  wb.reserve(wb_batch.size());
  for (auto& [sheet_name, batch] : wb_batch) {
    wb.emplace_back(std::move(sheet_name),
                    pluss::table::DataFrame(std::move(batch.features), std::move(batch.cases)));
  }
  return LoadResult(std::move(wb));
}

LoadResult load_excel(std::string_view filepath,
                      const std::vector<std::string>& sheet_names,
                      bool header) {
  ReadExcelOptions options;
  options.header = header;
  options.sheet_names = sheet_names;
  options.all_sheets = sheet_names.empty();
  auto wb_batch = ::pluss::loader::read_excel_workbook_batch(filepath, options);
  Workbook wb;
  wb.reserve(wb_batch.size());
  for (auto& [sheet_name, batch] : wb_batch) {
    wb.emplace_back(std::move(sheet_name),
                    pluss::table::DataFrame(std::move(batch.features), std::move(batch.cases)));
  }
  return LoadResult(std::move(wb));
}

LoadResult load_excel(std::string_view filepath,
                      std::initializer_list<std::string_view> sheet_names,
                      bool header) {
  std::vector<std::string> names;
  names.reserve(sheet_names.size());
  for (const auto sv : sheet_names) {
    names.emplace_back(sv);
  }
  return load_excel(filepath, names, header);
}

LoadResult load_excel(std::string_view filepath,
                      const std::vector<std::int64_t>& sheet_indices,
                      bool header) {
  ReadExcelOptions options;
  options.header = header;
  options.sheet_indices = sheet_indices;
  options.all_sheets = sheet_indices.empty();
  auto wb_batch = ::pluss::loader::read_excel_workbook_batch(filepath, options);
  Workbook wb;
  wb.reserve(wb_batch.size());
  for (auto& [sheet_name, batch] : wb_batch) {
    wb.emplace_back(std::move(sheet_name),
                    pluss::table::DataFrame(std::move(batch.features), std::move(batch.cases)));
  }
  return LoadResult(std::move(wb));
}

LoadResult load_excel(std::string_view filepath,
                      std::initializer_list<std::int64_t> sheet_indices,
                      bool header) {
  std::vector<std::int64_t> indices;
  indices.reserve(sheet_indices.size());
  for (const auto i : sheet_indices) {
    indices.push_back(i);
  }
  return load_excel(filepath, indices, header);
}

Workbook load_excel_workbook(std::string_view filepath, const Config& config) {
  auto wb_batch = read_excel_workbook_batch(filepath, config);
  Workbook wb;
  wb.reserve(wb_batch.size());
  for (auto& [sheet_name, batch] : wb_batch) {
    wb.emplace_back(std::move(sheet_name),
                    pluss::table::DataFrame(std::move(batch.features), std::move(batch.cases)));
  }
  return wb;
}

Workbook load_excel_workbook(std::string_view filepath,
                             std::initializer_list<ConfigEntry> config_entries) {
  return load_excel_workbook(filepath, config_from(config_entries));
}

CsvRawBatch read_parquet_raw_batch(std::string_view filepath, const Config& config) {
  PLUSS_CHECK(!trim_ascii(filepath).empty(), "loader::read_parquet_raw_batch: filepath cannot be empty");
#if PLUSSMIC_HAS_PARQUET_CPP
  return read_parquet_raw_batch_cpp(filepath, config);
#else
  (void)config;
  PLUSS_FAIL("loader::read_parquet_raw_batch: built without parquet-cpp support (install Arrow + Parquet C++ and reconfigure)");
#endif
}

CsvRawBatch read_parquet_raw_batch(std::string_view filepath,
                                   std::initializer_list<ConfigEntry> config_entries) {
  return read_parquet_raw_batch(filepath, config_from(config_entries));
}

CsvRawBatch read_json_raw_text(std::string_view json_text, const Config& config) {
  const auto root = JsonParser(json_text).parse();
  auto raw = raw_from_json_value(root, config);
  const auto options = csv_read_options_from_config(config);
  apply_projection_to_raw(raw, options);
  return raw;
}

CsvRawBatch read_json_raw_text(std::string_view json_text,
                               std::initializer_list<ConfigEntry> config_entries) {
  return read_json_raw_text(json_text, config_from(config_entries));
}

CsvRawBatch read_json_raw_batch(std::string_view filepath, const Config& config) {
  const auto text = read_text_file(filepath, "loader::read_json_raw_batch");
  return read_json_raw_text(text, config);
}

CsvRawBatch read_json_raw_batch(std::string_view filepath,
                                std::initializer_list<ConfigEntry> config_entries) {
  return read_json_raw_batch(filepath, config_from(config_entries));
}

pluss::table::DataFrame load_parquet(std::string_view filepath, const Config& config) {
  PLUSS_CHECK(!trim_ascii(filepath).empty(), "loader::load_parquet: filepath cannot be empty");
#if PLUSSMIC_HAS_PARQUET_CPP
  return load_parquet_cpp(filepath, config);
#else
  (void)config;
  PLUSS_FAIL("loader::load_parquet: built without parquet-cpp support (install Arrow + Parquet C++ and reconfigure)");
#endif
}

pluss::table::DataFrame load_parquet(std::string_view filepath,
                                     std::initializer_list<ConfigEntry> config_entries) {
  return load_parquet(filepath, config_from(config_entries));
}

pluss::table::DataFrame load_json(std::string_view filepath, const Config& config) {
  const auto options = csv_read_options_from_config(config);
  return dataframe_from_raw_batch_padded(read_json_raw_batch(filepath, config), options);
}

pluss::table::DataFrame load_json(std::string_view filepath,
                                  std::initializer_list<ConfigEntry> config_entries) {
  return load_json(filepath, config_from(config_entries));
}

LoadResult load(std::string_view source_type, std::string_view filepath, const Config& config) {
  const auto source = normalize_key(std::string(source_type));
  if (source == "csv") {
    return LoadResult(load_csv(filepath, config));
  }
  if (source == "txt" || source == "text") {
    return LoadResult(load_txt(filepath, config));
  }
  if (source == "excel" || source == "xlsx") {
    auto opts = excel_read_options_from_config(config);
    const bool workbook_mode =
        opts.all_sheets || !opts.sheet_names.empty() || !opts.sheet_indices.empty() || opts.n_sheets.has_value();
    if (workbook_mode) {
      auto wb_batch = ::pluss::loader::read_excel_workbook_batch(filepath, opts);
      Workbook wb;
      wb.reserve(wb_batch.size());
      for (auto& [sheet_name, batch] : wb_batch) {
        wb.emplace_back(std::move(sheet_name),
                        pluss::table::DataFrame(std::move(batch.features), std::move(batch.cases)));
      }
      return LoadResult(std::move(wb));
    }
    auto batch = ::pluss::loader::read_excel_batch(filepath, opts);
    return LoadResult(pluss::table::DataFrame(std::move(batch.features), std::move(batch.cases)));
  }
  if (source == "parquet") {
    return LoadResult(load_parquet(filepath, config));
  }
  if (source == "json") {
    return LoadResult(load_json(filepath, config));
  }
  PLUSS_FAIL("loader::load: unsupported source type '" + std::string(source_type) + "'");
}

LoadResult load(std::string_view source_type,
                std::string_view filepath,
                std::initializer_list<ConfigEntry> config_entries) {
  return load(source_type, filepath, config_from(config_entries));
}

std::string json_string_token(std::string_view value) {
  return "\"" + json_escape_text(value) + "\"";
}

std::string series_json_scalar_token(const pluss::Series& s, std::int64_t row) {
  const auto* base = s.raw_data();
  switch (s.scalar_type()) {
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
    case pluss::codem::ScalarType::Float16: {
      const auto v = pluss::codem::float16_to_float(reinterpret_cast<const pluss::codem::Float16*>(base)[row]);
      return std::isfinite(v) ? fmt::format("{:.9g}", v) : std::string("null");
    }
    case pluss::codem::ScalarType::BFloat16: {
      const auto v = pluss::codem::bfloat16_to_float(reinterpret_cast<const pluss::codem::BFloat16*>(base)[row]);
      return std::isfinite(v) ? fmt::format("{:.9g}", v) : std::string("null");
    }
    case pluss::codem::ScalarType::Float32: {
      const auto v = reinterpret_cast<const float*>(base)[row];
      return std::isfinite(v) ? fmt::format("{:.9g}", v) : std::string("null");
    }
    case pluss::codem::ScalarType::Float64: {
      const auto v = reinterpret_cast<const double*>(base)[row];
      return std::isfinite(v) ? fmt::format("{:.17g}", v) : std::string("null");
    }
    case pluss::codem::ScalarType::Complex64: {
      const auto v = reinterpret_cast<const pluss::codem::Complex64*>(base)[row];
      return json_string_token(fmt::format("({:.6g}{:+.6g}j)", v.real(), v.imag()));
    }
    case pluss::codem::ScalarType::Complex128: {
      const auto v = reinterpret_cast<const pluss::codem::Complex128*>(base)[row];
      return json_string_token(fmt::format("({:.12g}{:+.12g}j)", v.real(), v.imag()));
    }
    default:
      return "null";
  }
}

std::string thread_json_scalar_token(const pluss::table::Thread& thread,
                                     const std::optional<std::vector<bool>>& valid_mask,
                                     const std::optional<pluss::Note>& note_cache,
                                     std::int64_t row) {
  if (valid_mask.has_value() && !(*valid_mask)[static_cast<std::size_t>(row)]) {
    return "null";
  }

  switch (thread.kind()) {
    case pluss::table::ThreadKind::Series:
      return series_json_scalar_token(thread.as_series(), row);
    case pluss::table::ThreadKind::Note: {
      const auto* note = note_cache.has_value() ? &(*note_cache) : nullptr;
      if (note == nullptr) {
        const auto local = thread.as_note();
        return json_string_token(local.at(row));
      }
      return json_string_token(note->at(row));
    }
    case pluss::table::ThreadKind::Timeline:
    case pluss::table::ThreadKind::TimeIndex: {
      const auto* note = note_cache.has_value() ? &(*note_cache) : nullptr;
      if (note == nullptr) {
        const auto local = thread.cast(pluss::table::ThreadCastTarget::Note).as_note();
        return json_string_token(local.at(row));
      }
      return json_string_token(note->at(row));
    }
    default:
      return "null";
  }
}

enum class JsonSaveOrient : std::uint8_t {
  Records = 0,
  Columns = 1,
  Split = 2,
};

JsonSaveOrient json_orient_from_config(const Config& config) {
  if (const auto* v = find_config(config, {"orient", "json_orient"})) {
    if (auto s = parse_string_value(*v); s.has_value()) {
      const auto low = normalize_key(*s);
      if (low == "columns" || low == "column") return JsonSaveOrient::Columns;
      if (low == "split") return JsonSaveOrient::Split;
    }
  }
  return JsonSaveOrient::Records;
}

bool json_pretty_from_config(const Config& config) {
  if (const auto* v = find_config(config, {"pretty", "indent"})) {
    if (auto b = parse_bool_value(*v); b.has_value()) return *b;
    if (auto i = parse_int_value(*v); i.has_value()) return *i > 0;
  }
  return true;
}

std::string maybe_json_indent(const Config& config) {
  if (const auto* v = find_config(config, {"indent", "json_indent"})) {
    if (auto i = parse_int_value(*v); i.has_value() && *i > 0 && *i <= 8) {
      return std::string(static_cast<std::size_t>(*i), ' ');
    }
  }
  return "  ";
}

bool save_with_cases_from_config(const Config& config) {
  if (const auto* v = find_config(config, {"addcases", "cases", "case", "index"})) {
    if (auto b = parse_bool_value(*v); b.has_value()) return *b;
  }
  return false;
}

std::string save_case_label_from_config(const Config& config) {
  if (const auto* v = find_config(config, {"caselabel", "indexlabel", "case_feature"})) {
    if (auto s = parse_string_value(*v); s.has_value() && !s->empty()) return *s;
  }
  return "case";
}

std::vector<std::string> json_column_names_with_cases(const pluss::table::DataFrame& frame,
                                                      bool add_cases,
                                                      std::string_view case_label) {
  auto names = frame.feature_names();
  if (add_cases) {
    names.insert(names.begin(), std::string(case_label));
  }
  return names;
}

std::vector<pluss::table::Thread> json_columns_with_cases(const pluss::table::DataFrame& frame,
                                                          bool add_cases) {
  auto columns = frame.features();
  if (add_cases) {
    columns.insert(columns.begin(), frame.cases());
  }
  return columns;
}

void save_txt(const pluss::table::DataFrame& frame,
              std::string_view filepath,
              const Config& config) {
  auto options = csv_write_options_from_config(config);
  if (find_config(config, {"sep", "delimiter", "separator"}) == nullptr) {
    options.sep = '\t';
  }
  save_csv(frame, filepath, options);
}

void save_txt(const pluss::table::DataFrame& frame,
              std::string_view filepath,
              std::initializer_list<ConfigEntry> config_entries) {
  save_txt(frame, filepath, config_from(config_entries));
}

void save_csv(const pluss::table::DataFrame& frame,
              std::string_view filepath,
              const Config& config) {
  ::pluss::loader::save_csv(frame, filepath, csv_write_options_from_config(config));
}

void save_csv(const pluss::table::DataFrame& frame,
              std::string_view filepath,
              std::initializer_list<ConfigEntry> config_entries) {
  save_csv(frame, filepath, config_from(config_entries));
}

void save_excel(const pluss::table::DataFrame& frame,
                std::string_view filepath,
                const Config& config) {
  ::pluss::loader::save_excel(frame, filepath, excel_write_options_from_config(config));
}

void save_excel(const pluss::table::DataFrame& frame,
                std::string_view filepath,
                std::initializer_list<ConfigEntry> config_entries) {
  save_excel(frame, filepath, config_from(config_entries));
}

void save_json(const pluss::table::DataFrame& frame,
               std::string_view filepath,
               const Config& config) {
  const auto orient = json_orient_from_config(config);
  const auto pretty = json_pretty_from_config(config);
  const auto indent = maybe_json_indent(config);
  const auto add_cases = save_with_cases_from_config(config);
  const auto case_label = save_case_label_from_config(config);

  const auto names = json_column_names_with_cases(frame, add_cases, case_label);
  const auto columns = json_columns_with_cases(frame, add_cases);

  std::vector<std::optional<std::vector<bool>>> valid_masks(columns.size());
  std::vector<std::optional<pluss::Note>> note_cache(columns.size());
  for (std::size_t j = 0; j < columns.size(); ++j) {
    if (columns[j].has_validity_mask()) {
      valid_masks[j] = columns[j].validity_mask()->to_vec<bool>();
    }
    if (columns[j].kind() != pluss::table::ThreadKind::Series) {
      note_cache[j] = columns[j].cast(pluss::table::ThreadCastTarget::Note).as_note();
    }
  }

  std::ofstream out(std::string(filepath), std::ios::out | std::ios::trunc | std::ios::binary);
  PLUSS_CHECK(out.is_open(), "loader::save_json: failed to open file '" + std::string(filepath) + "'");

  const auto nrows = frame.ncases();
  const auto newline = pretty ? "\n" : "";
  const auto i1 = pretty ? indent : "";
  const auto i2 = pretty ? (indent + indent) : "";

  if (orient == JsonSaveOrient::Columns) {
    out << "{" << newline;
    for (std::size_t j = 0; j < columns.size(); ++j) {
      out << i1 << json_string_token(names[j]) << ":" << (pretty ? " " : "") << "[";
      for (std::int64_t r = 0; r < nrows; ++r) {
        if (r != 0) out << ",";
        if (pretty) out << " ";
        out << thread_json_scalar_token(columns[j], valid_masks[j], note_cache[j], r);
      }
      out << "]";
      if (j + 1 < columns.size()) out << ",";
      out << newline;
    }
    out << "}";
    return;
  }

  if (orient == JsonSaveOrient::Split) {
    out << "{" << newline;
    out << i1 << "\"columns\":" << (pretty ? " " : "") << "[";
    for (std::size_t j = 0; j < names.size(); ++j) {
      if (j != 0) out << ",";
      if (pretty) out << " ";
      out << json_string_token(names[j]);
    }
    out << "]," << newline;
    out << i1 << "\"data\":" << (pretty ? " " : "") << "[" << newline;
    for (std::int64_t r = 0; r < nrows; ++r) {
      out << i2 << "[";
      for (std::size_t j = 0; j < columns.size(); ++j) {
        if (j != 0) out << ",";
        if (pretty) out << " ";
        out << thread_json_scalar_token(columns[j], valid_masks[j], note_cache[j], r);
      }
      out << "]";
      if (r + 1 < nrows) out << ",";
      out << newline;
    }
    out << i1 << "]" << newline;
    out << "}";
    return;
  }

  // Records orientation (default).
  out << "[" << newline;
  for (std::int64_t r = 0; r < nrows; ++r) {
    out << i1 << "{" << newline;
    for (std::size_t j = 0; j < columns.size(); ++j) {
      out << i2 << json_string_token(names[j]) << ":" << (pretty ? " " : "");
      out << thread_json_scalar_token(columns[j], valid_masks[j], note_cache[j], r);
      if (j + 1 < columns.size()) out << ",";
      out << newline;
    }
    out << i1 << "}";
    if (r + 1 < nrows) out << ",";
    out << newline;
  }
  out << "]";
}

void save_json(const pluss::table::DataFrame& frame,
               std::string_view filepath,
               std::initializer_list<ConfigEntry> config_entries) {
  save_json(frame, filepath, config_from(config_entries));
}

void save_parquet(const pluss::table::DataFrame& frame,
                  std::string_view filepath,
                  const Config& config) {
#if PLUSSMIC_HAS_PARQUET_CPP
  save_parquet_cpp(frame, filepath, config);
#else
  (void)frame;
  (void)filepath;
  (void)config;
  PLUSS_FAIL("loader::save_parquet: built without parquet-cpp support (install Arrow + Parquet C++ and reconfigure)");
#endif
}

void save_parquet(const pluss::table::DataFrame& frame,
                  std::string_view filepath,
                  std::initializer_list<ConfigEntry> config_entries) {
  save_parquet(frame, filepath, config_from(config_entries));
}

void save_as(const pluss::table::DataFrame& frame,
             std::string_view target_type,
             std::string_view filepath,
             const Config& config) {
  const auto target = normalize_key(std::string(target_type));
  if (target == "csv") {
    save_csv(frame, filepath, config);
    return;
  }
  if (target == "excel" || target == "xlsx") {
    save_excel(frame, filepath, config);
    return;
  }
  if (target == "txt" || target == "text") {
    save_txt(frame, filepath, config);
    return;
  }
  if (target == "json") {
    save_json(frame, filepath, config);
    return;
  }
  if (target == "parquet") {
    save_parquet(frame, filepath, config);
    return;
  }
  PLUSS_FAIL("loader::save_as: unsupported target type '" + std::string(target_type) + "'");
}

void save_as(const pluss::table::DataFrame& frame,
             std::string_view target_type,
             std::string_view filepath,
             std::initializer_list<ConfigEntry> config_entries) {
  save_as(frame, target_type, filepath, config_from(config_entries));
}

} // namespace pluss::loader
