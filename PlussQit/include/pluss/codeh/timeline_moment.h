#pragma once

#include <array>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <exception>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/codeh/timeline_factory.h"
#include "pluss/detail/factory_containers.h"
#include "pluss/detail/nested_timeline_literal.h"

namespace pluss::timeline {

enum class MomentMode : std::uint8_t {
  Auto = 0,           // expression first, then strict datetime parse fallback
  ExpressionOnly = 1, // expression parser only
  Strict = 2,         // strict datetime parse only (equivalent to pluss::timeline::make)
};

enum class MomentClock : std::uint8_t {
  UTC = 0,
  Local = 1,
};

struct MomentOptions {
  MomentMode mode{MomentMode::Auto};
  MomentClock clock{MomentClock::UTC};
};

pluss::Timeline moment(std::string_view expr, const MomentOptions& options);

namespace detail {

inline std::string_view trim_ascii(std::string_view s) {
  std::size_t b = 0;
  std::size_t e = s.size();
  while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

inline std::string lower_ascii(std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (const unsigned char c : s) {
    out.push_back(static_cast<char>(std::tolower(c)));
  }
  return out;
}

inline std::string compact_ascii(std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (const unsigned char c : s) {
    if (!std::isspace(c)) {
      out.push_back(static_cast<char>(c));
    }
  }
  return out;
}

inline bool is_leap_year(int year) {
  if (year % 400 == 0) return true;
  if (year % 100 == 0) return false;
  return (year % 4 == 0);
}

inline int days_in_month(int year, int month) {
  static constexpr int kDays[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (month == 2) {
    return is_leap_year(year) ? 29 : 28;
  }
  return kDays[month - 1];
}

inline std::array<int, 3> current_ymd(MomentClock clock) {
  const auto now_t = std::time(nullptr);
  std::tm tm{};
#if defined(_WIN32)
  if (clock == MomentClock::UTC) {
    gmtime_s(&tm, &now_t);
  } else {
    localtime_s(&tm, &now_t);
  }
#else
  if (clock == MomentClock::UTC) {
    gmtime_r(&now_t, &tm);
  } else {
    localtime_r(&now_t, &tm);
  }
#endif
  return {tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday};
}

inline std::string ymd_to_iso(int year, int month, int day) {
  char buf[11];
  std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year, month, day);
  return std::string(buf);
}

inline std::int64_t scalar_epoch_nanos(const pluss::Timeline& t, std::string_view where) {
  const auto flat = t.to_epoch_nanos_vector();
  CODEM_CHECK(flat.size() == 1, std::string(where) + ": expected scalar timeline");
  return flat[0];
}

inline std::int64_t current_now_ns() {
  const auto now = std::chrono::system_clock::now();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
}

inline bool parse_offset_ns(std::string_view token, std::int64_t& out_ns) {
  out_ns = 0;
  if (token.empty()) {
    return true;
  }
  if (token.size() < 3 || (token[0] != '+' && token[0] != '-')) {
    return false;
  }

  std::size_t i = 1;
  std::int64_t magnitude = 0;
  bool saw_digit = false;
  while (i < token.size() && std::isdigit(static_cast<unsigned char>(token[i]))) {
    saw_digit = true;
    magnitude = magnitude * 10 + static_cast<std::int64_t>(token[i] - '0');
    ++i;
  }
  if (!saw_digit || i >= token.size()) {
    return false;
  }

  const auto unit = token.substr(i);
  std::int64_t unit_ns = 0;
  if (unit == "s") {
    unit_ns = 1'000'000'000LL;
  } else if (unit == "m") {
    unit_ns = 60LL * 1'000'000'000LL;
  } else if (unit == "h") {
    unit_ns = 3600LL * 1'000'000'000LL;
  } else if (unit == "d") {
    unit_ns = 24LL * 3600LL * 1'000'000'000LL;
  } else if (unit == "w") {
    unit_ns = 7LL * 24LL * 3600LL * 1'000'000'000LL;
  } else {
    return false;
  }

  out_ns = magnitude * unit_ns;
  if (token[0] == '-') {
    out_ns = -out_ns;
  }
  return true;
}

inline bool parse_expression_ns(std::string_view expr, const MomentOptions& options, std::int64_t& out_ns) {
  const auto trimmed = trim_ascii(expr);
  if (trimmed.empty()) {
    return false;
  }
  const auto compact = lower_ascii(compact_ascii(trimmed));

  std::size_t split = std::string::npos;
  for (std::size_t i = 1; i < compact.size(); ++i) {
    if (compact[i] == '+' || compact[i] == '-') {
      split = i;
      break;
    }
  }
  const auto base = (split == std::string::npos) ? compact : compact.substr(0, split);
  const auto offset_text = (split == std::string::npos) ? std::string_view{} : std::string_view(compact).substr(split);

  std::int64_t base_ns = 0;
  if (base == "now") {
    base_ns = current_now_ns();
  } else {
    const auto ymd = current_ymd(options.clock);
    int year = ymd[0];
    int month = ymd[1];
    int day = ymd[2];

    if (base == "today") {
      // already set
    } else if (base == "yesterday") {
      base_ns = scalar_epoch_nanos(pluss::timeline::make(ymd_to_iso(year, month, day)), "moment(yesterday)")
          - 24LL * 3600LL * 1'000'000'000LL;
    } else if (base == "tomorrow") {
      base_ns = scalar_epoch_nanos(pluss::timeline::make(ymd_to_iso(year, month, day)), "moment(tomorrow)")
          + 24LL * 3600LL * 1'000'000'000LL;
    } else if (base == "som" || base == "startofmonth" || base == "start_of_month") {
      day = 1;
      base_ns = scalar_epoch_nanos(pluss::timeline::make(ymd_to_iso(year, month, day)), "moment(start_of_month)");
    } else if (base == "eom" || base == "endofmonth" || base == "end_of_month") {
      day = days_in_month(year, month);
      base_ns = scalar_epoch_nanos(pluss::timeline::make(ymd_to_iso(year, month, day)), "moment(end_of_month)");
    } else {
      return false;
    }

    if (base == "today") {
      base_ns = scalar_epoch_nanos(pluss::timeline::make(ymd_to_iso(year, month, day)), "moment(today)");
    }
  }

  std::int64_t offset_ns = 0;
  if (!parse_offset_ns(offset_text, offset_ns)) {
    return false;
  }
  out_ns = base_ns + offset_ns;
  return true;
}

inline pluss::Timeline moment_from_textspan(::pluss::detail::TextSpan s1d, const MomentOptions& options) {
  if (options.mode == MomentMode::Strict) {
    const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(s1d.size())};
    return pluss::codeh::ops_internal::timeline_from_utf8(s1d.span(), sizes);
  }
  if (options.mode == MomentMode::Auto) {
    try {
      const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(s1d.size())};
      return pluss::codeh::ops_internal::timeline_from_utf8(s1d.span(), sizes);
    } catch (const std::exception&) {
      // Fallback to expression-aware parsing below.
    }
  }

  std::vector<std::int64_t> ticks;
  ticks.reserve(s1d.size());
  for (const auto& item : s1d.span()) {
    ticks.push_back(detail::scalar_epoch_nanos(pluss::timeline::moment(item, options), "moment(TextSpan)"));
  }
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(ticks.size())};
  return pluss::codeh::ops_internal::timeline_from_epoch_nanos(
      std::span<const std::int64_t>(ticks.data(), ticks.size()), sizes);
}

} // namespace detail

inline pluss::Timeline moment(std::string_view expr, const MomentOptions& options = {}) {
  if (options.mode == MomentMode::Strict) {
    return pluss::timeline::make(expr);
  }

  std::int64_t epoch_ns = 0;
  if (detail::parse_expression_ns(expr, options, epoch_ns)) {
    return pluss::timeline::from_epoch(epoch_ns);
  }

  CODEM_CHECK(options.mode != MomentMode::ExpressionOnly,
              "pluss::timeline::moment: unsupported moment expression '" + std::string(expr) + "'");
  return pluss::timeline::make(expr);
}

inline pluss::Timeline moment(const char* expr, const MomentOptions& options = {}) {
  return moment(std::string_view{expr ? expr : ""}, options);
}

inline pluss::Timeline moment(pluss::msharp::RangeView::Of<std::string_view> s1d, const MomentOptions& options = {}) {
  return detail::moment_from_textspan(
      ::pluss::detail::TextSpan(std::span<const std::string_view>(s1d.data(), s1d.size())),
      options);
}

inline pluss::Timeline moment(pluss::msharp::RangeView::Of<std::string> s1d, const MomentOptions& options = {}) {
  return detail::moment_from_textspan(
      ::pluss::detail::TextSpan::from_strings(std::span<const std::string>(s1d.data(), s1d.size())),
      options);
}

inline pluss::Timeline moment(std::initializer_list<::pluss::detail::NestedTimelineLiteral> nested,
                              const MomentOptions& options = {}) {
  auto lit = ::pluss::detail::flatten_timeline_literal(nested, 3, "pluss::timeline::moment");
  std::vector<std::int64_t> ticks;
  ticks.reserve(lit.flat.size());
  for (const auto& item : lit.flat) {
    ticks.push_back(detail::scalar_epoch_nanos(moment(item, options), "moment(nested)"));
  }
  return pluss::codeh::ops_internal::timeline_from_epoch_nanos(
      std::span<const std::int64_t>(ticks.data(), ticks.size()),
      std::span<const std::int64_t>(lit.sizes.data(), lit.sizes.size()));
}

// pluss::timeline::now(clock) -> scalar Timeline at the current instant.
// Convenience wrapper over moment("now", ...) for the single most common
// moment expression -- no string literal or MomentOptions boilerplate needed
// at the call site.
inline pluss::Timeline now(MomentClock clock = MomentClock::UTC) {
  return moment("now", MomentOptions{MomentMode::Auto, clock});
}

// pluss::timeline::today(clock) -> scalar Timeline at the start of today
// (midnight). Convenience wrapper over moment("today", ...).
inline pluss::Timeline today(MomentClock clock = MomentClock::UTC) {
  return moment("today", MomentOptions{MomentMode::Auto, clock});
}

} // namespace pluss::timeline
