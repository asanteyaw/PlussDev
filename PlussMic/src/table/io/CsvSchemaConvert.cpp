#include "table/io/CsvSchemaConvert.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <limits>
#include <mutex>
#include <span>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "CodeM/core/Errors.h"
#include "pluss/coden/note_factory.h"
#include "pluss/codeh/timeline_factory.h"

namespace pluss::table::detail {

namespace {

using Thread = pluss::table::Thread;

enum class InferredKind : std::uint8_t {
  Bool,
  Int64,
  Float64,
  DateTime,
  Text,
};

double elapsed_ms(std::chrono::steady_clock::time_point start,
                  std::chrono::steady_clock::time_point stop) {
  return std::chrono::duration<double, std::milli>(stop - start).count();
}

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

bool all_ascii_digits(std::string_view s) {
  if (s.empty()) {
    return false;
  }
  for (const char ch : s) {
    if (!std::isdigit(static_cast<unsigned char>(ch))) {
      return false;
    }
  }
  return true;
}

bool parse_int_sv(std::string_view s, int& out) {
  if (!all_ascii_digits(s)) {
    return false;
  }
  int value = 0;
  for (const char ch : s) {
    value = value * 10 + (ch - '0');
  }
  out = value;
  return true;
}

bool parse_time_part(std::string_view part, int& hh, int& mm, int& ss, int& ns) {
  hh = 0;
  mm = 0;
  ss = 0;
  ns = 0;

  const auto p1 = part.find(':');
  if (p1 == std::string_view::npos) {
    return false;
  }
  const auto p2 = part.find(':', p1 + 1);

  const auto h_sv = part.substr(0, p1);
  const auto m_sv = (p2 == std::string_view::npos)
      ? part.substr(p1 + 1)
      : part.substr(p1 + 1, p2 - (p1 + 1));
  if (!parse_int_sv(h_sv, hh) || !parse_int_sv(m_sv, mm)) {
    return false;
  }

  if (p2 != std::string_view::npos) {
    auto sec_frac = part.substr(p2 + 1);
    const auto dot = sec_frac.find('.');
    const auto s_sv = dot == std::string_view::npos ? sec_frac : sec_frac.substr(0, dot);
    if (!parse_int_sv(s_sv, ss)) {
      return false;
    }
    if (dot != std::string_view::npos) {
      auto frac = sec_frac.substr(dot + 1);
      if (frac.empty() || !all_ascii_digits(frac)) {
        return false;
      }
      if (frac.size() > 9) {
        frac = frac.substr(0, 9);
      }
      int frac_ns = 0;
      for (const char ch : frac) {
        frac_ns = frac_ns * 10 + (ch - '0');
      }
      for (std::size_t i = frac.size(); i < 9; ++i) {
        frac_ns *= 10;
      }
      ns = frac_ns;
    }
  }

  if (hh < 0 || hh > 23 || mm < 0 || mm > 59 || ss < 0 || ss > 60 || ns < 0 || ns > 999'999'999) {
    return false;
  }
  return true;
}

bool parse_date_part(std::string_view part, bool dayfirst, int& y, int& mo, int& d) {
  const auto delim = part.find('/') != std::string_view::npos
      ? '/'
      : (part.find('-') != std::string_view::npos ? '-' : (part.find('.') != std::string_view::npos ? '.' : '\0'));
  if (delim == '\0') {
    return false;
  }

  const auto p1 = part.find(delim);
  if (p1 == std::string_view::npos) {
    return false;
  }
  const auto p2 = part.find(delim, p1 + 1);
  if (p2 == std::string_view::npos || part.find(delim, p2 + 1) != std::string_view::npos) {
    return false;
  }

  const auto a = part.substr(0, p1);
  const auto b = part.substr(p1 + 1, p2 - (p1 + 1));
  const auto c = part.substr(p2 + 1);
  int ai = 0;
  int bi = 0;
  int ci = 0;
  if (!parse_int_sv(a, ai) || !parse_int_sv(b, bi) || !parse_int_sv(c, ci)) {
    return false;
  }

  if (a.size() == 4) {
    y = ai;
    mo = bi;
    d = ci;
    return true;
  }
  if (c.size() == 4) {
    y = ci;
    const bool choose_dayfirst = (ai > 12 && bi <= 12) ? true
        : ((bi > 12 && ai <= 12) ? false : dayfirst);
    if (choose_dayfirst) {
      d = ai;
      mo = bi;
    } else {
      mo = ai;
      d = bi;
    }
    return true;
  }
  return false;
}

bool parse_tz_suffix(std::string_view s, std::string_view& main, bool& has_tz, int& tz_offset_min) {
  has_tz = false;
  tz_offset_min = 0;
  main = s;
  if (s.empty()) {
    return true;
  }

  const auto last = s.back();
  if (last == 'Z' || last == 'z') {
    has_tz = true;
    tz_offset_min = 0;
    main = s.substr(0, s.size() - 1);
    return true;
  }

  if (s.size() >= 6) {
    const auto sign_idx = s.size() - 6;
    const char sign = s[sign_idx];
    if ((sign == '+' || sign == '-') && s[s.size() - 3] == ':') {
      const auto hh = s.substr(sign_idx + 1, 2);
      const auto mm = s.substr(sign_idx + 4, 2);
      int h = 0;
      int m = 0;
      if (!parse_int_sv(hh, h) || !parse_int_sv(mm, m)) {
        return false;
      }
      has_tz = true;
      tz_offset_min = (sign == '-' ? -1 : 1) * (h * 60 + m);
      main = s.substr(0, sign_idx);
      return true;
    }
  }

  if (s.size() >= 5) {
    const auto sign_idx = s.size() - 5;
    const char sign = s[sign_idx];
    if (sign == '+' || sign == '-') {
      const auto hh = s.substr(sign_idx + 1, 2);
      const auto mm = s.substr(sign_idx + 3, 2);
      int h = 0;
      int m = 0;
      if (!parse_int_sv(hh, h) || !parse_int_sv(mm, m)) {
        return false;
      }
      has_tz = true;
      tz_offset_min = (sign == '-' ? -1 : 1) * (h * 60 + m);
      main = s.substr(0, sign_idx);
      return true;
    }
  }

  return true;
}

std::optional<std::int64_t> parse_datetime_like_epoch_nanos(std::string_view token, bool dayfirst) {
  const auto trimmed = trim_ascii(token);
  if (trimmed.empty()) {
    return std::nullopt;
  }

  std::string_view main = trimmed;
  bool has_tz = false;
  int tz_offset_min = 0;
  if (!parse_tz_suffix(main, main, has_tz, tz_offset_min)) {
    return std::nullopt;
  }

  const auto sep = main.find_first_of("Tt ");
  std::string_view date_part;
  std::string_view time_part;

  if (sep == std::string_view::npos) {
    if (main.find(':') != std::string_view::npos) {
      date_part = "1970-01-01";
      time_part = main;
    } else {
      date_part = main;
    }
  } else {
    date_part = main.substr(0, sep);
    time_part = main.substr(sep + 1);
    while (!time_part.empty() && std::isspace(static_cast<unsigned char>(time_part.front()))) {
      time_part.remove_prefix(1);
    }
  }

  int y = 1970;
  int mo = 1;
  int d = 1;
  if (!parse_date_part(date_part, dayfirst, y, mo, d)) {
    return std::nullopt;
  }

  int hh = 0;
  int mm = 0;
  int ss = 0;
  int ns = 0;
  if (!time_part.empty() && !parse_time_part(time_part, hh, mm, ss, ns)) {
    return std::nullopt;
  }

  const auto ymd = std::chrono::year(y) / std::chrono::month(static_cast<unsigned>(mo)) / std::chrono::day(static_cast<unsigned>(d));
  if (!ymd.ok()) {
    return std::nullopt;
  }

  auto tp = std::chrono::sys_days(ymd)
      + std::chrono::hours(hh)
      + std::chrono::minutes(mm)
      + std::chrono::seconds(ss)
      + std::chrono::nanoseconds(ns);
  if (has_tz) {
    tp -= std::chrono::minutes(tz_offset_min);
  }
  return tp.time_since_epoch().count();
}

bool looks_datetime_feature_name(std::string_view name) {
  const auto lower = lower_copy(trim_ascii(name));
  return lower.find("date") != std::string::npos ||
      lower.find("time") != std::string::npos ||
      lower.find("timestamp") != std::string::npos ||
      lower.find("datetime") != std::string::npos;
}

bool looks_boolean_feature_name(std::string_view name) {
  const auto lower = lower_copy(trim_ascii(name));
  return lower == "bool" ||
      lower == "boolean" ||
      lower == "flag" ||
      lower.starts_with("is_") ||
      lower.starts_with("has_") ||
      lower.starts_with("was_") ||
      lower.starts_with("can_") ||
      lower.ends_with("_flag") ||
      lower.ends_with("_bool") ||
      lower.find("active") != std::string::npos ||
      lower.find("enabled") != std::string::npos;
}

bool looks_datetime_value(std::string_view token) {
  const auto trimmed = trim_ascii(token);
  if (trimmed.empty()) {
    return false;
  }
  bool has_digit = false;
  bool has_datetime_sep = false;
  for (const char c : trimmed) {
    if (std::isdigit(static_cast<unsigned char>(c))) {
      has_digit = true;
      continue;
    }
    if (c == '-' || c == '/' || c == ':' || c == 'T' || c == 'Z' || c == ' ') {
      has_datetime_sep = true;
    }
  }
  return has_digit && has_datetime_sep;
}

bool raw_values_look_datetime_like(const std::vector<std::string>& values, const std::vector<bool>& valid) {
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (valid[i] && looks_datetime_value(values[i])) {
      return true;
    }
  }
  return false;
}

bool raw_values_look_datetime_like(const pluss::coden::Note& values, const std::vector<bool>& valid) {
  for (std::size_t i = 0; i < static_cast<std::size_t>(values.numel()); ++i) {
    if (valid[i] && looks_datetime_value(values.at(static_cast<std::int64_t>(i)))) {
      return true;
    }
  }
  return false;
}

bool ambiguous_numeric_bool_tokens(const std::vector<std::string>& values, const std::vector<bool>& valid) {
  bool saw_value = false;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (!valid[i]) {
      continue;
    }
    saw_value = true;
    const auto token = trim_ascii(values[i]);
    if (token != "0" && token != "1") {
      return false;
    }
  }
  return saw_value;
}

bool ambiguous_numeric_bool_tokens(const pluss::coden::Note& values, const std::vector<bool>& valid) {
  bool saw_value = false;
  for (std::size_t i = 0; i < static_cast<std::size_t>(values.numel()); ++i) {
    if (!valid[i]) {
      continue;
    }
    saw_value = true;
    const auto token = trim_ascii(values.at(static_cast<std::int64_t>(i)));
    if (token != "0" && token != "1") {
      return false;
    }
  }
  return saw_value;
}

bool is_na_token(std::string_view token, const pluss::loader::ReadCsvOptions& options) {
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

bool parse_bool_token(std::string_view token, const pluss::loader::ReadCsvOptions& options, bool& out) {
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

std::string normalize_numeric(std::string_view token, const pluss::loader::ReadCsvOptions& options) {
  std::string s = trim_ascii(token);
  if (options.thousands.has_value()) {
    s.erase(std::remove(s.begin(), s.end(), *options.thousands), s.end());
  }
  if (options.decimal != '.') {
    std::replace(s.begin(), s.end(), options.decimal, '.');
  }
  return s;
}

bool parse_int64_token(std::string_view token, const pluss::loader::ReadCsvOptions& options, std::int64_t& out) {
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

bool parse_double_token(std::string_view token, const pluss::loader::ReadCsvOptions& options, double& out) {
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
  auto note = pluss::make_note(values);
  const bool all_valid = std::all_of(valid.begin(), valid.end(), [](bool value) { return value; });
  if (all_valid) {
    return Thread(note);
  }
  return Thread(note, validity_mask_from_valid(valid));
}

bool try_make_timeline_thread(const std::vector<std::string>& values,
                              const std::vector<bool>& valid,
                              const pluss::loader::ReadCsvOptions& options,
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
    // Fall back to a permissive parser for common CSV date/time formats:
    // dd/mm/yyyy, mm/dd/yyyy, yyyy-mm-dd, HH:MM[:SS[.frac]].
    std::vector<std::int64_t> ticks(values.size(), 0);
    for (std::size_t i = 0; i < values.size(); ++i) {
      if (!valid[i]) {
        continue;
      }
      const auto parsed = parse_datetime_like_epoch_nanos(values[i], options.dayfirst);
      if (!parsed.has_value()) {
        return false;
      }
      ticks[i] = *parsed;
    }
    auto timeline = pluss::codeh::ops_internal::timeline_from_epoch_nanos(
        std::span<const std::int64_t>(ticks.data(), ticks.size()),
        std::span<const std::int64_t>(shape.data(), shape.size()));
    out_thread = Thread(timeline, validity_mask_from_valid(valid));
    return true;
  }
}

bool try_make_timeline_thread(const pluss::coden::Note& values,
                              const std::vector<bool>& valid,
                              const pluss::loader::ReadCsvOptions& options,
                              Thread& out_thread) {
  const auto n = static_cast<std::size_t>(values.numel());
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(n)};

  std::vector<std::string_view> views(n);
  constexpr std::string_view kNullFill = "1970-01-01T00:00:00Z";
  for (std::size_t i = 0; i < n; ++i) {
    views[i] = valid[i] ? values.at(static_cast<std::int64_t>(i)) : kNullFill;
  }

  try {
    auto timeline = pluss::codeh::ops_internal::timeline_from_utf8(
        std::span<const std::string_view>(views.data(), views.size()),
        std::span<const std::int64_t>(shape.data(), shape.size()));
    out_thread = Thread(timeline, validity_mask_from_valid(valid));
    return true;
  } catch (...) {
    std::vector<std::int64_t> ticks(n, 0);
    for (std::size_t i = 0; i < n; ++i) {
      if (!valid[i]) {
        continue;
      }
      const auto parsed = parse_datetime_like_epoch_nanos(values.at(static_cast<std::int64_t>(i)), options.dayfirst);
      if (!parsed.has_value()) {
        return false;
      }
      ticks[i] = *parsed;
    }
    auto timeline = pluss::codeh::ops_internal::timeline_from_epoch_nanos(
        std::span<const std::int64_t>(ticks.data(), ticks.size()),
        std::span<const std::int64_t>(shape.data(), shape.size()));
    out_thread = Thread(timeline, validity_mask_from_valid(valid));
    return true;
  }
}

bool should_parse_as_datetime(const std::string& name,
                              std::size_t index,
                              const pluss::loader::ReadCsvOptions& options,
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
  if (options.infer_datetime_format && looks_datetime_feature_name(name)) {
    return true;
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
                           const pluss::loader::ReadCsvOptions& options,
                           const std::unordered_map<std::string, std::string>& dtype_lower,
                           bool prefer_numeric_over_bool = false) {
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

  const bool parse_dt =
      (!forced.has_value() || *forced == InferredKind::DateTime)
      && should_parse_as_datetime(col_name, source_col_index, options, dtype_lower);
  if (parse_dt || (forced.has_value() && *forced == InferredKind::DateTime)) {
    Thread out;
    if (try_make_timeline_thread(values, valid, options, out)) {
      return out;
    }
    if (forced.has_value() && *forced == InferredKind::DateTime) {
      PLUSS_FAIL("load_csv: failed to parse datetime column '" + col_name + "'");
    }
  }

  const auto try_bool_block = [&]() -> std::optional<Thread> {
    const bool try_bool = !forced.has_value() || *forced == InferredKind::Bool;
    if (!try_bool) return std::nullopt;
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
      if (ambiguous_numeric_bool_tokens(values, valid) && !looks_boolean_feature_name(col_name)) {
        return std::nullopt;
      }
      return make_series_thread_bool(parsed, valid);
    }
    if (forced.has_value() && *forced == InferredKind::Bool) {
      PLUSS_FAIL("load_csv: failed to parse bool column '" + col_name + "'");
    }
    return std::nullopt;
  };

  const auto try_i64_block = [&]() -> std::optional<Thread> {
    const bool try_i64 = !forced.has_value() || *forced == InferredKind::Int64;
    if (!try_i64) return std::nullopt;
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
    return std::nullopt;
  };

  if (prefer_numeric_over_bool) {
    if (auto t = try_i64_block()) return *t;
    if (auto t = try_bool_block()) return *t;
  } else {
    if (auto t = try_bool_block()) return *t;
    if (auto t = try_i64_block()) return *t;
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

  if (raw_values_look_datetime_like(values, valid)) {
    Thread out;
    if (try_make_timeline_thread(values, valid, options, out)) {
      return out;
    }
  }

  return make_note_thread(values, valid);
}

Thread infer_column_note_thread(const std::string& col_name,
                                std::size_t source_col_index,
                                const pluss::coden::Note& raw_values,
                                const pluss::loader::ReadCsvOptions& options,
                                const std::unordered_map<std::string, std::string>& dtype_lower,
                                bool prefer_numeric_over_bool = false) {
  const auto n = static_cast<std::size_t>(raw_values.numel());
  std::vector<bool> valid(n, true);

  for (std::size_t i = 0; i < n; ++i) {
    if (is_na_token(raw_values.at(static_cast<std::int64_t>(i)), options)) {
      valid[i] = false;
    }
  }

  std::optional<InferredKind> forced;
  (void)forced_kind_for(col_name, dtype_lower, forced);

  const bool parse_dt =
      (!forced.has_value() || *forced == InferredKind::DateTime)
      && should_parse_as_datetime(col_name, source_col_index, options, dtype_lower);
  if (parse_dt || (forced.has_value() && *forced == InferredKind::DateTime)) {
    Thread out;
    if (try_make_timeline_thread(raw_values, valid, options, out)) {
      return out;
    }
    if (forced.has_value() && *forced == InferredKind::DateTime) {
      PLUSS_FAIL("load_csv: failed to parse datetime column '" + col_name + "'");
    }
  }

  const auto try_bool_block = [&]() -> std::optional<Thread> {
    const bool try_bool = !forced.has_value() || *forced == InferredKind::Bool;
    if (!try_bool) return std::nullopt;
    std::vector<bool> parsed(n, false);
    bool ok = true;
    for (std::size_t i = 0; i < n; ++i) {
      if (!valid[i]) continue;
      bool v = false;
      if (!parse_bool_token(raw_values.at(static_cast<std::int64_t>(i)), options, v)) {
        ok = false;
        break;
      }
      parsed[i] = v;
    }
    if (ok) {
      if (ambiguous_numeric_bool_tokens(raw_values, valid) && !looks_boolean_feature_name(col_name)) {
        return std::nullopt;
      }
      return make_series_thread_bool(parsed, valid);
    }
    if (forced.has_value() && *forced == InferredKind::Bool) {
      PLUSS_FAIL("load_csv: failed to parse bool column '" + col_name + "'");
    }
    return std::nullopt;
  };

  const auto try_i64_block = [&]() -> std::optional<Thread> {
    const bool try_i64 = !forced.has_value() || *forced == InferredKind::Int64;
    if (!try_i64) return std::nullopt;
    std::vector<std::int64_t> parsed(n, 0);
    bool ok = true;
    for (std::size_t i = 0; i < n; ++i) {
      if (!valid[i]) continue;
      std::int64_t v = 0;
      if (!parse_int64_token(raw_values.at(static_cast<std::int64_t>(i)), options, v)) {
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
    return std::nullopt;
  };

  if (prefer_numeric_over_bool) {
    if (auto t = try_i64_block()) return *t;
    if (auto t = try_bool_block()) return *t;
  } else {
    if (auto t = try_bool_block()) return *t;
    if (auto t = try_i64_block()) return *t;
  }

  {
    const bool try_f64 = !forced.has_value() || *forced == InferredKind::Float64;
    if (try_f64) {
      std::vector<double> parsed(n, 0.0);
      bool ok = true;
      for (std::size_t i = 0; i < n; ++i) {
        if (!valid[i]) continue;
        double v = 0.0;
        if (!parse_double_token(raw_values.at(static_cast<std::int64_t>(i)), options, v)) {
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

  if (raw_values_look_datetime_like(raw_values, valid)) {
    Thread out;
    if (try_make_timeline_thread(raw_values, valid, options, out)) {
      return out;
    }
  }

  std::vector<std::string> values;
  values.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    values.emplace_back(raw_values.at(static_cast<std::int64_t>(i)));
  }
  return make_note_thread(values, valid);
}

std::size_t resolve_column_workers(const pluss::loader::ReadCsvOptions& options, std::size_t column_count) {
  if (column_count < 2) {
    return 1;
  }
  std::size_t threads = 0;
  if (options.parallel_threads.has_value() && *options.parallel_threads > 0) {
    threads = *options.parallel_threads;
  } else {
    threads = static_cast<std::size_t>(std::thread::hardware_concurrency());
  }
  if (threads == 0) {
    threads = 1;
  }
  return std::max<std::size_t>(1, std::min<std::size_t>(threads, column_count));
}

} // namespace

std::unordered_map<std::string, std::string> dtype_lower_from_options(const pluss::loader::ReadCsvOptions& options) {
  std::unordered_map<std::string, std::string> dtype_lower;
  dtype_lower.reserve(options.dtype.size());
  for (const auto& kv : options.dtype) {
    dtype_lower.emplace(kv.first, lower_copy(kv.second));
  }
  return dtype_lower;
}

pluss::table::Thread infer_thread_from_raw_csv_column(std::string_view col_name,
                                                       std::size_t source_col_index,
                                                       const std::vector<std::string>& raw_values,
                                                       const pluss::loader::ReadCsvOptions& options,
                                                       bool prefer_numeric_over_bool) {
  auto dtype_lower = dtype_lower_from_options(options);
  return infer_column_thread(std::string(col_name), source_col_index, raw_values, options, dtype_lower,
                             prefer_numeric_over_bool);
}

pluss::table::Thread infer_thread_from_raw_csv_column(const pluss::loader::CsvRawColumn& column,
                                                       const pluss::loader::ReadCsvOptions& options,
                                                       bool prefer_numeric_over_bool) {
  auto dtype_lower = dtype_lower_from_options(options);
  if (column.note_values.has_value()) {
    return infer_column_note_thread(column.name,
                                    column.source_index,
                                    *column.note_values,
                                    options,
                                    dtype_lower,
                                    prefer_numeric_over_bool);
  }
  return infer_column_thread(column.name, column.source_index, column.values, options, dtype_lower,
                             prefer_numeric_over_bool);
}

pluss::loader::CsvBatch csv_batch_from_raw_batch(const pluss::loader::CsvRawBatch& raw,
                                                 const pluss::loader::ReadCsvOptions& options,
                                                 double* infer_columns_ms) {
  const auto t_infer_start = std::chrono::steady_clock::now();

  std::vector<pluss::table::Thread> inferred(raw.features.size());
  const auto max_workers = resolve_column_workers(options, raw.features.size());
  const bool use_parallel = max_workers > 1 && raw.features.size() >= 4;

  if (use_parallel) {
    std::atomic<std::size_t> next_index{0};
    std::atomic<bool> failed{false};
    std::exception_ptr first_error;
    std::mutex error_mutex;

    auto worker = [&]() {
      while (true) {
        if (failed.load(std::memory_order_relaxed)) {
          return;
        }
        const auto i = next_index.fetch_add(1, std::memory_order_relaxed);
        if (i >= raw.features.size()) {
          return;
        }
        try {
          inferred[i] = infer_thread_from_raw_csv_column(raw.features[i], options);
        } catch (...) {
          {
            std::lock_guard<std::mutex> lock(error_mutex);
            if (!first_error) {
              first_error = std::current_exception();
            }
          }
          failed.store(true, std::memory_order_relaxed);
          return;
        }
      }
    };

    std::vector<std::thread> workers;
    workers.reserve(max_workers > 0 ? max_workers - 1 : 0);
    for (std::size_t t = 1; t < max_workers; ++t) {
      workers.emplace_back(worker);
    }
    worker();
    for (auto& w : workers) {
      if (w.joinable()) {
        w.join();
      }
    }
    if (first_error) {
      std::rethrow_exception(first_error);
    }
  } else {
    for (std::size_t i = 0; i < raw.features.size(); ++i) {
      inferred[i] = infer_thread_from_raw_csv_column(raw.features[i], options);
    }
  }

  if (infer_columns_ms != nullptr) {
    *infer_columns_ms = elapsed_ms(t_infer_start, std::chrono::steady_clock::now());
  }

  std::vector<std::pair<std::string, pluss::table::Thread>> cols;
  cols.reserve(raw.features.size());
  for (std::size_t i = 0; i < raw.features.size(); ++i) {
    cols.emplace_back(raw.features[i].name, std::move(inferred[i]));
  }

  std::optional<pluss::table::Thread> cases = std::nullopt;
  if (raw.cases.has_value()) {
    cases = infer_thread_from_raw_csv_column(*raw.cases, options);
  }

  return pluss::loader::CsvBatch{std::move(cols), std::move(cases)};
}

} // namespace pluss::table::detail
