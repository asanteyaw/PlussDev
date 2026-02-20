#include "loader/Loader.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include "CodeM/core/Errors.h"
#include "table/DataFrame.h"

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

  if (const auto* v = find_config(config, {"nrows", "rows"})) {
    if (auto n = parse_int_value(*v); n.has_value() && *n >= 0) {
      options.nrows = static_cast<std::size_t>(*n);
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

  if (const auto* v = find_config(config, {"usecols", "columns", "selectcols"})) {
    if (auto cols = parse_string_list_value(*v); cols.has_value()) {
      options.usecols = std::move(*cols);
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

  if (const auto* v = find_config(config, {"sheetname", "sheet", "sheetindex"})) {
    if (v->is_none()) {
      options.all_sheets = true;
    } else if (auto idx = parse_int_value(*v); idx.has_value()) {
      options.sheet_index = *idx;
    } else if (auto names = parse_string_list_value(*v); names.has_value() && !names->empty()) {
      options.sheet_names = std::move(*names);
      options.all_sheets = false;
      options.sheet_name.reset();
      options.sheet_index.reset();
    } else if (auto s = parse_string_value(*v); s.has_value()) {
      if (is_none_token(*s)) {
        options.all_sheets = true;
      } else {
        options.sheet_name = *s;
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

  return options;
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

} // namespace

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
    os << fmt::format("\n  [{}] {} -> [{} cs x {} feat]", i, name, frame.nrows(), frame.ncols());
  }
  return os;
}

pluss::table::DataFrame load_csv(std::string_view filepath, const Config& config) {
  return ::pluss::loader::load_csv(filepath, csv_read_options_from_config(config));
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
    options.nrows = *ncases;
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

pluss::table::DataFrame load_excel(std::string_view filepath, const Config& config) {
  return ::pluss::loader::load_excel(filepath, excel_read_options_from_config(config));
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
  return LoadResult(::pluss::loader::load_excel_workbook(filepath, options));
}

LoadResult load_excel(std::string_view filepath,
                      const std::vector<std::string>& sheet_names,
                      bool header) {
  ReadExcelOptions options;
  options.header = header;
  options.sheet_names = sheet_names;
  options.all_sheets = sheet_names.empty();
  return LoadResult(::pluss::loader::load_excel_workbook(filepath, options));
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

Workbook load_excel_workbook(std::string_view filepath, const Config& config) {
  auto opts = excel_read_options_from_config(config);
  if (!opts.all_sheets && opts.sheet_names.empty() && !opts.n_sheets.has_value()) {
    opts.all_sheets = true;
  }
  return ::pluss::loader::load_excel_workbook(filepath, opts);
}

Workbook load_excel_workbook(std::string_view filepath,
                             std::initializer_list<ConfigEntry> config_entries) {
  return load_excel_workbook(filepath, config_from(config_entries));
}

pluss::table::DataFrame load_parquet(std::string_view filepath, const Config& config) {
  (void)filepath;
  (void)config;
  PLUSS_FAIL("loader::load_parquet: parquet loader is not implemented yet");
}

pluss::table::DataFrame load_parquet(std::string_view filepath,
                                     std::initializer_list<ConfigEntry> config_entries) {
  return load_parquet(filepath, config_from(config_entries));
}

LoadResult load(std::string_view source_type, std::string_view filepath, const Config& config) {
  const auto source = normalize_key(std::string(source_type));
  if (source == "csv") {
    return LoadResult(load_csv(filepath, config));
  }
  if (source == "excel" || source == "xlsx") {
    auto opts = excel_read_options_from_config(config);
    const bool workbook_mode =
        opts.all_sheets || !opts.sheet_names.empty() || opts.n_sheets.has_value();
    if (workbook_mode) {
      return LoadResult(::pluss::loader::load_excel_workbook(filepath, opts));
    }
    return LoadResult(::pluss::loader::load_excel(filepath, opts));
  }
  if (source == "parquet") {
    return LoadResult(load_parquet(filepath, config));
  }
  PLUSS_FAIL("loader::load: unsupported source type '" + std::string(source_type) + "'");
}

LoadResult load(std::string_view source_type,
                std::string_view filepath,
                std::initializer_list<ConfigEntry> config_entries) {
  return load(source_type, filepath, config_from(config_entries));
}

void save_as(const pluss::table::DataFrame& frame,
             std::string_view target_type,
             std::string_view filepath,
             const Config& config) {
  const auto target = normalize_key(std::string(target_type));
  if (target == "csv") {
    save_csv(frame, filepath, csv_write_options_from_config(config));
    return;
  }
  if (target == "excel" || target == "xlsx") {
    save_excel(frame, filepath, excel_write_options_from_config(config));
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
