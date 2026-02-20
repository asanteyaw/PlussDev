#include "loader/CsvLoader.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include "CodeM/core/Complex.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/ScalarType.h"
#include "table/DataFrame.h"
#include "table/Thread.h"
#include "loader/parallel_csv_reader.h"

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

char effective_separator(const ReadCsvOptions& options) {
  if (options.delimiter.has_value()) {
    return *options.delimiter;
  }
  return options.sep;
}

char sniff_separator_from_sample(std::string_view sample, std::optional<char> quotechar, char fallback) {
  constexpr std::array<char, 5> kCandidates{',', ';', '\t', '|', ':'};
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
                              const ReadCsvOptions& options,
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
    if (try_make_timeline_thread(values, valid, options, out)) {
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
    if (try_make_timeline_thread(values, valid, options, out)) {
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

std::vector<std::vector<std::string>> normalize_input_records(
    const std::vector<std::vector<std::string>>& records,
    const ReadCsvOptions& options) {
  std::vector<std::vector<std::string>> out;
  out.reserve(records.size());

  for (std::size_t logical_row = 0; logical_row < records.size(); ++logical_row) {
    if (options.skiprows_first_n > 0 && logical_row < options.skiprows_first_n) {
      continue;
    }
    if (options.skiprows.find(logical_row) != options.skiprows.end()) {
      continue;
    }

    auto row = records[logical_row];
    if (options.skipinitialspace) {
      for (auto& f : row) {
        ltrim_inplace(f);
      }
    }
    out.push_back(std::move(row));
  }

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

std::string thread_value_for_csv(const pluss::table::Thread& t, std::int64_t row, const WriteCsvOptions& options) {
  if (t.has_validity_mask()) {
    const auto valid = t.validity_mask()->to_vec<bool>();
    if (!valid[static_cast<std::size_t>(row)]) {
      return options.na_rep;
    }
  }

  switch (t.kind()) {
    case pluss::table::ThreadKind::Series:
      return scalar_for_csv(t.as_series(), row, options);
    case pluss::table::ThreadKind::Note:
      return std::string(t.as_note().at(row));
    case pluss::table::ThreadKind::Timeline:
      return fmt::format("{}", t.as_timeline().to_epoch_nanos_vector()[static_cast<std::size_t>(row)]);
    case pluss::table::ThreadKind::TimeIndex:
      return fmt::format("{}",
                         t.as_timeindex().timeline().to_epoch_nanos_vector()[static_cast<std::size_t>(row)]);
    default:
      return "";
  }
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

} // namespace

pluss::table::DataFrame load_csv_records(const std::vector<std::vector<std::string>>& records,
                                         const ReadCsvOptions& options) {
  auto normalized = normalize_input_records(records, options);
  PLUSS_CHECK(!normalized.empty(), "load_csv: no rows parsed from csv");

  std::vector<std::string> names;
  std::size_t data_start = 0;

  if (options.header) {
    PLUSS_CHECK(options.header_row >= 0, "load_csv: header_row must be >= 0");
    const auto h = static_cast<std::size_t>(options.header_row);
    PLUSS_CHECK(h < normalized.size(), "load_csv: header_row is out of range");
    names = normalized[h];
    data_start = h + 1;
  } else {
    const auto ncol = normalized.front().size();
    names.reserve(ncol);
    for (std::size_t i = 0; i < ncol; ++i) {
      names.push_back(fmt::format("feat_{}", i));
    }
    data_start = 0;
  }

  if (!options.names.empty()) {
    names = options.names;
  }

  PLUSS_CHECK(!names.empty(), "load_csv: inferred zero columns");

  if (options.skipfooter > 0) {
    const auto n_data = normalized.size() > data_start ? (normalized.size() - data_start) : 0;
    PLUSS_CHECK(options.skipfooter <= n_data, "load_csv: skipfooter exceeds number of data rows");
  }

  const auto usecols = resolve_usecols(names, options);

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
  selected_cols.reserve(usecols.size());
  for (const auto c : usecols) {
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

  std::vector<std::vector<std::string>> values_by_col(selected_cols.size());
  std::vector<std::string> case_values;

  std::size_t bad_lines = 0;
  std::size_t accepted_rows = 0;

  const std::size_t tail_cut = options.skipfooter;
  const std::size_t final_end = normalized.size() - tail_cut;

  for (std::size_t r = data_start; r < final_end; ++r) {
    const auto& rec = normalized[r];

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

    if (case_col.has_value()) {
      case_values.push_back(rec[*case_col]);
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

  std::optional<pluss::table::Thread> cases = std::nullopt;
  if (case_col.has_value()) {
    const auto case_name = names[*case_col];
    cases = infer_column_thread(case_name, *case_col, case_values, options, dtype_lower);
  }

  return pluss::table::DataFrame(std::move(cols), std::move(cases));
}

pluss::table::DataFrame load_csv(const ReadCsvOptions& options) {
  PLUSS_CHECK(!options.filepath.empty(), "load_csv: filepath is empty");

  ReadCsvOptions resolved = options;
  resolved.sep = detect_separator(options);

  std::vector<std::vector<std::string>> records;
  auto parse_serial = [&]() {
    std::ifstream in(resolved.filepath, std::ios::in | std::ios::binary);
    PLUSS_CHECK(in.is_open(), "load_csv: failed to open file '" + resolved.filepath + "'");
    std::vector<std::vector<std::string>> out;
    out.reserve(4096);
    std::vector<std::string> row;
    while (parse_record(in, resolved, row)) {
      out.push_back(row);
    }
    return out;
  };

  std::error_code fs_ec;
  const auto file_bytes = std::filesystem::file_size(resolved.filepath, fs_ec);
  const auto known_size = fs_ec ? static_cast<std::size_t>(0) : static_cast<std::size_t>(file_bytes);

  const bool attempt_parallel = ParallelCsvReader::should_use_parallel(known_size, resolved);
  if (attempt_parallel) {
    try {
      records = ParallelCsvReader::read_file(resolved);
    } catch (const std::exception& e) {
      if (resolved.engine == CsvEngine::Parallel) {
        throw;
      }
      ::pluss::codem::warn(
          fmt::format("load_csv: parallel parse fallback to serial for '{}' ({})", resolved.filepath, e.what()));
      records = parse_serial();
    }
  } else {
    records = parse_serial();
  }

  return load_csv_records(records, resolved);
}

pluss::table::DataFrame load_csv(std::string_view filepath, const ReadCsvOptions& options) {
  ReadCsvOptions resolved = options;
  resolved.filepath = std::string(filepath);
  return load_csv(resolved);
}

pluss::table::DataFrame load_csv(std::string_view filepath,
                                 char sep,
                                 std::optional<std::size_t> nrows,
                                 std::vector<std::string> usecols,
                                 std::optional<std::string> case_col,
                                 BadLinePolicy on_bad_lines) {
  ReadCsvOptions options;
  options.sep = sep;
  options.nrows = nrows;
  options.usecols = std::move(usecols);
  options.case_col = std::move(case_col);
  options.on_bad_lines = on_bad_lines;
  return load_csv(filepath, options);
}

pluss::table::DataFrame load_csv(std::string_view filepath,
                                 char sep,
                                 std::optional<std::size_t> nrows,
                                 std::initializer_list<std::string_view> usecols,
                                 std::optional<std::string> case_col,
                                 BadLinePolicy on_bad_lines) {
  std::vector<std::string> cols;
  cols.reserve(usecols.size());
  for (const auto name : usecols) {
    cols.emplace_back(name);
  }
  return load_csv(filepath, sep, nrows, std::move(cols), std::move(case_col), on_bad_lines);
}

void save_csv(const pluss::table::DataFrame& frame,
              std::string_view filepath,
              const WriteCsvOptions& options) {
  PLUSS_CHECK(frame.defined(), "save_csv: input frame is undefined");
  PLUSS_CHECK(!filepath.empty(), "save_csv: filepath is empty");

  std::ofstream out(std::string(filepath), std::ios::out | std::ios::trunc | std::ios::binary);
  PLUSS_CHECK(out.is_open(), "save_csv: failed to open output file '" + std::string(filepath) + "'");

  const auto sep = options.sep;
  const auto names = frame.column_names();
  const auto cols = frame.columns();
  const bool include_cases = options.add_cases || options.cases;

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

  for (std::int64_t r = 0; r < frame.nrows(); ++r) {
    bool first = true;
    if (include_cases) {
      out << escape_csv_cell(thread_value_for_csv(frame.cases(), r, options), options);
      first = false;
    }
    for (const auto& col : cols) {
      if (!first) {
        out << sep;
      }
      out << escape_csv_cell(thread_value_for_csv(col, r, options), options);
      first = false;
    }
    out << options.line_terminator;
  }
}

} // namespace pluss::loader
