
#include "CodeH/core/Timeline.h"
#include "CodeH/core/TimelineIndexAccessor.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cctype>
#include <cstring>
#include <cmath>
#include <iomanip>
#include <limits>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "CodeH/core/Offsets.h"
#include "CodeH/core/TimeZone.h"
#include "CodeH/core/detail/Int64ScalarOp.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesIterator.h"
#include "pluss/coden/note_factory.h"

namespace pluss::codeh {

using namespace std::chrono;

// ---------------------------
// Small ISO-ish parser
// ---------------------------

namespace {

static void check_timeline_core(const pluss::codem::Series& s) {
  TimelineBase::validate_ticks_series(s);
}

static pluss::codem::Series make_i64_series(const std::vector<std::int64_t>& values) {
  auto out = pluss::codem::Series::empty(
      {static_cast<std::int64_t>(values.size())},
      pluss::codem::ScalarType::Int64);
  if (!values.empty()) {
    std::memcpy(out.raw_data(), values.data(), values.size() * sizeof(std::int64_t));
  }
  return out;
}

struct ParsedDT {
  bool ok{false};
  int y{1970}, mo{1}, d{1};
  int hh{0}, mm{0}, ss{0};
  int ns{0}; // 0..999,999,999
  bool has_tz{false};
  int tz_offset_min{0}; // minutes east of UTC
};

static bool parse_int(const std::string& s, size_t i, size_t n, int& out) {
  if (i + n > s.size()) return false;
  int v = 0;
  for (size_t k = 0; k < n; ++k) {
    unsigned char c = (unsigned char)s[i + k];
    if (!std::isdigit(c)) return false;
    v = v * 10 + (c - '0');
  }
  out = v;
  return true;
}

static std::string trim(std::string_view sv) {
  size_t b = 0, e = sv.size();
  while (b < e && std::isspace((unsigned char)sv[b])) ++b;
  while (e > b && std::isspace((unsigned char)sv[e - 1])) --e;
  return std::string(sv.substr(b, e - b));
}

// Supported examples:
//  - 2026-02-15
//  - 2026/02/15 13:04
//  - 2026-02-15T13:04:05
//  - 2026-02-15T13:04:05.123456789Z
//  - 2026-02-15 13:04:05-07:00
static ParsedDT parse_datetime(std::string_view in) {
  ParsedDT p;
  const std::string s = trim(in);
  if (s.empty()) return p;

  // Split off timezone suffix if present.
  // Match: Z, +HH:MM, -HH:MM, +HHMM, -HHMM
  std::string main = s;
  {
    static const std::regex tz_re(R"((.*?)(Z|[+\-]\d{2}:?\d{2})$)");
    std::smatch m;
    if (std::regex_match(s, m, tz_re)) {
      main = m[1].str();
      const std::string tz = m[2].str();
      if (tz == "Z") {
        p.has_tz = true;
        p.tz_offset_min = 0;
      } else {
        int sign = (tz[0] == '-') ? -1 : 1;
        int hh = 0, mm = 0;
        if (tz.size() == 6 && tz[3] == ':') { // +HH:MM
          hh = std::stoi(tz.substr(1, 2));
          mm = std::stoi(tz.substr(4, 2));
        } else if (tz.size() == 5) { // +HHMM
          hh = std::stoi(tz.substr(1, 2));
          mm = std::stoi(tz.substr(3, 2));
        } else {
          return p;
        }
        if (hh > 23 || mm > 59) return p;
        p.has_tz = true;
        p.tz_offset_min = sign * (hh * 60 + mm);
      }
    }
  }

  // Normalize separators in date part.
  std::string date_part;
  std::string time_part;
  {
    size_t tpos = main.find('T');
    size_t spos = main.find(' ');
    size_t sep = std::min(tpos == std::string::npos ? main.size() : tpos,
                          spos == std::string::npos ? main.size() : spos);
    if (sep == main.size()) {
      date_part = main;
    } else {
      date_part = main.substr(0, sep);
      time_part = main.substr(sep + 1);
    }
  }

  if (date_part.size() < 8) return p;
  for (auto& c : date_part) if (c == '/') c = '-';
  if (date_part.size() != 10 || date_part[4] != '-' || date_part[7] != '-') return p;
  if (!parse_int(date_part, 0, 4, p.y)) return p;
  if (!parse_int(date_part, 5, 2, p.mo)) return p;
  if (!parse_int(date_part, 8, 2, p.d)) return p;

  if (!time_part.empty()) {
    if (time_part.size() < 5 || time_part[2] != ':') return p;
    if (!parse_int(time_part, 0, 2, p.hh) ||
        !parse_int(time_part, 3, 2, p.mm)) return p;
    p.ss = 0;
    p.ns = 0;

    if (time_part.size() > 5) {
      if (time_part[5] != ':') return p;
      if (time_part.size() < 8) return p;
      if (!parse_int(time_part, 6, 2, p.ss)) return p;
      if (time_part.size() > 8) {
        if (time_part[8] != '.') return p;
        std::string frac = time_part.substr(9);
        if (frac.empty()) return p;
        if (frac.size() > 9) frac = frac.substr(0, 9);
        while (frac.size() < 9) frac.push_back('0');
        for (char c : frac) if (!std::isdigit((unsigned char)c)) return p;
        p.ns = std::stoi(frac);
      }
    }
  }

  const auto ymd = std::chrono::year(p.y) / std::chrono::month((unsigned)p.mo) / std::chrono::day((unsigned)p.d);
  if (!ymd.ok()) return p;
  if (p.hh < 0 || p.hh > 23) return p;
  if (p.mm < 0 || p.mm > 59) return p;
  if (p.ss < 0 || p.ss > 59) return p;
  if (p.ns < 0 || p.ns > 999'999'999) return p;

  p.ok = true;
  return p;
}

static std::int64_t to_epoch_nanos_utc(const ParsedDT& p) {
  const auto day = std::chrono::sys_days(std::chrono::year(p.y) / std::chrono::month((unsigned)p.mo) / std::chrono::day((unsigned)p.d));
  constexpr __int128 ns_per_second = 1'000'000'000LL;
  constexpr __int128 ns_per_day = 86'400LL * ns_per_second;
  __int128 value = static_cast<__int128>(day.time_since_epoch().count()) * ns_per_day;
  value += static_cast<__int128>(p.hh * 3600 + p.mm * 60 + p.ss) * ns_per_second;
  value += p.ns;
  if (p.has_tz) {
    value -= static_cast<__int128>(p.tz_offset_min) * 60 * ns_per_second;
  }
  CODEM_CHECK(value > std::numeric_limits<std::int64_t>::min() &&
                  value <= std::numeric_limits<std::int64_t>::max(),
              "moment: datetime is outside the nanosecond Timeline range");
  return static_cast<std::int64_t>(value);
}

static inline std::int64_t unit_nanos(std::string_view unit) {
  if (unit == "S" || unit == "s" || unit == "sec" || unit == "second") return 1'000'000'000LL;
  if (unit == "min" || unit == "m" || unit == "minute") return 60LL * 1'000'000'000LL;
  if (unit == "H" || unit == "h" || unit == "hour") return 3600LL * 1'000'000'000LL;
  if (unit == "D" || unit == "d" || unit == "day") return 86400LL * 1'000'000'000LL;
  CODEM_FAIL("Timeline: unsupported unit (expected S/min/H/D)");
}

static inline bool is_day_unit(std::string_view unit) {
  return unit == "D" || unit == "d" || unit == "day";
}

static void epoch_nanos_to_local_fields(std::int64_t ns_utc, const TimeZone& tz,
                                        int& y, int& mo, int& d, int& hh, int& mm, int& ss, int& nsec,
                                        int* out_offset_min = nullptr) {
  const int off = tz.offset_minutes_at_utc(ns_utc);
  if (out_offset_min) *out_offset_min = off;
  const auto tp = sys_time<nanoseconds>(nanoseconds(ns_utc + (std::int64_t)off * 60LL * 1'000'000'000LL));
  const auto dp = floor<days>(tp);
  const auto ymd = year_month_day(dp);
  y = int(ymd.year());
  mo = unsigned(ymd.month());
  d = unsigned(ymd.day());
  const auto tod = tp - dp;
  const auto h = floor<hours>(tod);
  const auto m = floor<minutes>(tod - h);
  const auto s = floor<seconds>(tod - h - m);
  const auto n = tod - h - m - s;
  hh = int(h.count());
  mm = int(m.count());
  ss = int(s.count());
  nsec = int(n.count());
}

static std::string zoffset_str(int offset_min) {
  // +HH:MM
  int sign = offset_min < 0 ? -1 : 1;
  int v = std::abs(offset_min);
  int hh = v / 60;
  int mm = v % 60;
  std::ostringstream oss;
  oss << (sign < 0 ? '-' : '+') << std::setw(2) << std::setfill('0') << hh << ":" << std::setw(2) << std::setfill('0') << mm;
  return oss.str();
}

struct FormatPiece {
  bool directive{false};
  char code{};
  std::string literal;
};

static std::vector<FormatPiece> compile_format(std::string_view fmt) {
  std::vector<FormatPiece> pieces;
  std::string literal;
  const auto flush_literal = [&] {
    if (!literal.empty()) {
      pieces.push_back(FormatPiece{false, 0, std::move(literal)});
      literal.clear();
    }
  };
  constexpr std::string_view supported{"%YmdHMSFTRfzAaBbjuwqZN"};
  for (std::size_t i = 0; i < fmt.size(); ++i) {
    if (fmt[i] != '%') {
      literal.push_back(fmt[i]);
      continue;
    }
    CODEM_CHECK(i + 1 < fmt.size(), "pluss::timeline::format: trailing '%'");
    const char code = fmt[++i];
    CODEM_CHECK(supported.find(code) != std::string_view::npos,
                std::string("pluss::timeline::format: unsupported directive %") + code);
    flush_literal();
    pieces.push_back(FormatPiece{true, code, {}});
  }
  flush_literal();
  return pieces;
}

static void append_padded(std::string& out, int value, int width) {
  char digits[32];
  if (value < 0) {
    out.push_back('-');
    value = -value;
  }
  const auto result = std::to_chars(std::begin(digits), std::end(digits), value);
  const auto count = static_cast<int>(result.ptr - digits);
  out.append(static_cast<std::size_t>(std::max(0, width - count)), '0');
  out.append(digits, result.ptr);
}

static std::string format_one(std::int64_t ns_utc, const TimeZone& tz,
                              std::string_view tz_name,
                              const std::vector<FormatPiece>& pieces,
                              std::string_view nat_text, int fractional_digits,
                              bool trim_fractional_zeros) {
  if (ns_utc == Timeline::kNaT) return std::string(nat_text);
  int y, mo, d, hh, mm, ss, nsec, off_min;
  epoch_nanos_to_local_fields(ns_utc, tz, y, mo, d, hh, mm, ss, nsec, &off_min);
  std::string out;
  out.reserve(32);
  const auto dp = sys_days(year(y)/month(static_cast<unsigned>(mo))/day(static_cast<unsigned>(d)));
  const int sunday0 = static_cast<int>(weekday(dp).c_encoding());
  const int monday0 = (sunday0 + 6) % 7;
  const int year_day = static_cast<int>((dp - sys_days(year(y)/January/1)).count()) + 1;
  for (const auto& piece : pieces) {
    if (!piece.directive) {
      out += piece.literal;
      continue;
    }
    const char t = piece.code;
    switch (t) {
      case '%': out.push_back('%'); break;
      case 'Y': append_padded(out, y, 4); break;
      case 'm': append_padded(out, mo, 2); break;
      case 'd': append_padded(out, d, 2); break;
      case 'H': append_padded(out, hh, 2); break;
      case 'M': append_padded(out, mm, 2); break;
      case 'S': append_padded(out, ss, 2); break;
      case 'F':
        append_padded(out, y, 4); out.push_back('-');
        append_padded(out, mo, 2); out.push_back('-'); append_padded(out, d, 2);
        break;
      case 'T':
        append_padded(out, hh, 2); out.push_back(':'); append_padded(out, mm, 2);
        out.push_back(':'); append_padded(out, ss, 2);
        break;
      case 'R':
        append_padded(out, hh, 2); out.push_back(':'); append_padded(out, mm, 2);
        break;
      case 'f':
      case 'N': {
        char fraction[10];
        std::snprintf(fraction, sizeof(fraction), "%09d", nsec);
        std::string_view shown(fraction, static_cast<std::size_t>(fractional_digits));
        if (trim_fractional_zeros) {
          while (shown.size() > 1 && shown.back() == '0') shown.remove_suffix(1);
        }
        out.append(shown);
      } break;
      case 'z': out += zoffset_str(off_min); break;
      case 'Z': out += tz_name; break;
      case 'j': append_padded(out, year_day, 3); break;
      case 'u': out += std::to_string(monday0 + 1); break;
      case 'w': out += std::to_string(sunday0); break;
      case 'q': out += std::to_string((mo - 1) / 3 + 1); break;
      case 'a': { // short weekday
        static const char* names[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
        out += names[monday0];
      } break;
      case 'A': {
        static const char* names[] = {"Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday"};
        out += names[monday0];
      } break;
      case 'b': {
        static const char* names[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
        out += names[mo-1];
      } break;
      case 'B': {
        static const char* names[] = {"January","February","March","April","May","June","July","August","September","October","November","December"};
        out += names[mo-1];
      } break;
      default: CODEM_FAIL("pluss::timeline::format: invalid compiled directive");
    }
  }
  return out;
}

static std::string strftime_one(std::int64_t ns_utc, const TimeZone& tz, std::string_view fmt) {
  return format_one(ns_utc, tz, "UTC", compile_format(fmt), "NaT", 9, false);
}

} // namespace

// ---------------------------
// ops_internal factories
// ---------------------------

namespace ops_internal {

Timeline timeline_from_epoch_nanos(std::span<const std::int64_t> ticks_flat,
                                  std::span<const std::int64_t> sizes) {
  // sizes empty => 0D scalar
  std::vector<std::int64_t> shape(sizes.begin(), sizes.end());
  if (shape.empty()) {
    CODEM_CHECK(ticks_flat.size() == 1, "from_epoch_nanos: scalar expects 1 element");
  } else {
    std::int64_t prod = 1;
    for (auto s : shape) prod *= s;
    CODEM_CHECK(prod == (std::int64_t)ticks_flat.size(), "from_epoch_nanos: flat size mismatch");
  }
  if (ticks_flat.empty()) {
    auto s = pluss::codem::Series::empty(shape, pluss::codem::ScalarType::Int64);
    check_timeline_core(s);
    return Timeline(std::move(s), "UTC");
  }
  auto s = pluss::codem::Series::empty(
      std::span<const std::int64_t>(shape.data(), shape.size()),
      pluss::codem::ScalarType::Int64);
  std::memcpy(s.raw_data(), ticks_flat.data(),
              ticks_flat.size() * sizeof(std::int64_t));
  check_timeline_core(s);
  return Timeline(std::move(s), "UTC");
}

Timeline timeline_from_series(pluss::codem::Series ticks, std::string tz_name) {
  CODEM_CHECK(ticks.scalar_type() == pluss::codem::ScalarType::Int64,
              "timeline_from_series: expected an Int64 epoch-nanoseconds Series");
  check_timeline_core(ticks);
  return Timeline(std::move(ticks), std::move(tz_name));
}

Timeline timeline_from_utf8(std::span<const std::string_view> utf8_flat,
                            std::span<const std::int64_t> sizes) {
  std::vector<std::int64_t> shape(sizes.begin(), sizes.end());
  if (shape.empty()) {
    CODEM_CHECK(utf8_flat.size() == 1, "moment: scalar expects 1 string");
  } else {
    std::int64_t prod = 1;
    for (auto s : shape) prod *= s;
    CODEM_CHECK(prod == (std::int64_t)utf8_flat.size(), "moment: flat size mismatch");
  }

  std::vector<std::int64_t> ticks(utf8_flat.size());
  std::transform(utf8_flat.begin(), utf8_flat.end(), ticks.begin(),
                 [](std::string_view sv) {
                   // An empty (or whitespace-only) string is the documented way to
                   // request a NaT row from text input -- distinct from a genuinely
                   // malformed, non-empty string, which still throws below.
                   if (trim(sv).empty()) {
                     return Timeline::kNaT;
                   }
                   const auto p = parse_datetime(sv);
                   CODEM_CHECK(p.ok, "moment: failed to parse datetime");
                   return to_epoch_nanos_utc(p);
                 });
  return timeline_from_epoch_nanos(
      std::span<const std::int64_t>(ticks.data(), ticks.size()),
      std::span<const std::int64_t>(shape.data(), shape.size()));
}

} // namespace ops_internal

// ---------------------------
// detail impls (used by invoke)
// ---------------------------

namespace detail {

namespace {
std::int64_t checked_tick_add(std::int64_t tick, std::int64_t delta,
                              std::string_view where) {
  constexpr auto min_tick = std::numeric_limits<std::int64_t>::min() + 1;
  constexpr auto max_tick = std::numeric_limits<std::int64_t>::max();
  CODEM_CHECK((delta <= 0 || tick <= max_tick - delta) &&
                  (delta >= 0 || tick >= min_tick - delta),
              std::string(where) + ": result is outside Timeline range");
  return tick + delta;
}

std::int64_t checked_tick_sub(std::int64_t tick, std::int64_t delta,
                              std::string_view where) {
  constexpr auto min_tick = std::numeric_limits<std::int64_t>::min() + 1;
  constexpr auto max_tick = std::numeric_limits<std::int64_t>::max();
  CODEM_CHECK((delta >= 0 || tick <= max_tick + delta) &&
                  (delta <= 0 || tick >= min_tick + delta),
              std::string(where) + ": result is outside Timeline range");
  return tick - delta;
}
} // namespace

Timeline impl_add_duration(const Timeline& a, const Duration& d) {
  check_timeline_core(a.ticks_);
  CODEM_CHECK(d.defined(), "Timeline + Duration: duration undefined");
  CODEM_CHECK(d.nanos_.scalar_type() == pluss::codem::ScalarType::Int64, "Timeline + Duration: nanos must be int64");
  const auto shape = pluss::codem::SeriesIterator::broadcast_shape(a.ticks_.sizes(), d.nanos_.sizes());
  auto out = pluss::codem::Series::empty(shape, pluss::codem::ScalarType::Int64);
  if (std::ranges::find(shape, 0) != shape.end()) {
    return Timeline(std::move(out), std::string(a.tz()));
  }
  auto it = pluss::codem::SeriesIterator::binary(a.ticks_, d.nanos_, out);
  const auto* ap = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  const auto* bp = reinterpret_cast<const std::int64_t*>(it.b().raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t bo, std::int64_t oo) {
    const auto av = ap[ao];
    const auto bv = bp[bo];
    op[oo] = (av == Timeline::kNaT || bv == Duration::kNaT)
        ? Timeline::kNaT
        : checked_tick_add(av, bv, "Timeline + Duration");
  });
  return Timeline(std::move(out), std::string(a.tz()));
}

Timeline impl_sub_duration(const Timeline& a, const Duration& d) {
  check_timeline_core(a.ticks_);
  CODEM_CHECK(d.defined(), "Timeline - Duration: duration undefined");
  CODEM_CHECK(d.nanos_.scalar_type() == pluss::codem::ScalarType::Int64, "Timeline - Duration: nanos must be int64");
  const auto shape = pluss::codem::SeriesIterator::broadcast_shape(a.ticks_.sizes(), d.nanos_.sizes());
  auto out = pluss::codem::Series::empty(shape, pluss::codem::ScalarType::Int64);
  if (std::ranges::find(shape, 0) != shape.end()) {
    return Timeline(std::move(out), std::string(a.tz()));
  }
  auto it = pluss::codem::SeriesIterator::binary(a.ticks_, d.nanos_, out);
  const auto* ap = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  const auto* bp = reinterpret_cast<const std::int64_t*>(it.b().raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t bo, std::int64_t oo) {
    const auto av = ap[ao];
    const auto bv = bp[bo];
    op[oo] = (av == Timeline::kNaT || bv == Duration::kNaT)
        ? Timeline::kNaT
        : checked_tick_sub(av, bv, "Timeline - Duration");
  });
  return Timeline(std::move(out), std::string(a.tz()));
}

Duration impl_diff(const Timeline& a, const Timeline& b) {
  check_timeline_core(a.ticks_);
  check_timeline_core(b.ticks_);
  const auto shape = pluss::codem::SeriesIterator::broadcast_shape(a.ticks_.sizes(), b.ticks_.sizes());
  auto out = pluss::codem::Series::empty(shape, pluss::codem::ScalarType::Int64);
  if (std::ranges::find(shape, 0) != shape.end()) {
    return Duration(out);
  }
  auto it = pluss::codem::SeriesIterator::binary(a.ticks_, b.ticks_, out);
  const auto* ap = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  const auto* bp = reinterpret_cast<const std::int64_t*>(it.b().raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t bo, std::int64_t oo) {
    const auto av = ap[ao];
    const auto bv = bp[bo];
    op[oo] = (av == Timeline::kNaT || bv == Timeline::kNaT)
        ? Duration::kNaT
        : checked_tick_sub(av, bv, "Timeline - Timeline");
  });
  return Duration(out);
}

pluss::codem::Series impl_cmp(const Timeline& a, const Timeline& b, std::string_view op) {
  check_timeline_core(a.ticks_);
  check_timeline_core(b.ticks_);
  const auto shape = pluss::codem::SeriesIterator::broadcast_shape(a.ticks_.sizes(), b.ticks_.sizes());
  auto out = pluss::codem::Series::empty(shape, pluss::codem::ScalarType::Bool);
  if (std::ranges::find(shape, 0) != shape.end()) {
    return out;
  }
  auto it = pluss::codem::SeriesIterator::binary(a.ticks_, b.ticks_, out);
  const auto* ap = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  const auto* bp = reinterpret_cast<const std::int64_t*>(it.b().raw_data());
  auto* optr = reinterpret_cast<bool*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t bo, std::int64_t oo) {
    const auto av = ap[ao];
    const auto bv = bp[bo];
    if (av == Timeline::kNaT || bv == Timeline::kNaT) {
      // NaT follows the usual missing-instant comparison contract: it is
      // unequal to every value, including itself, and unordered otherwise.
      optr[oo] = (op == "ne");
      return;
    }
    bool r = false;
    if (op == "eq") r = (av == bv);
    else if (op == "ne") r = (av != bv);
    else if (op == "lt") r = (av < bv);
    else if (op == "le") r = (av <= bv);
    else if (op == "gt") r = (av > bv);
    else if (op == "ge") r = (av >= bv);
    else CODEM_FAIL("Timeline compare: unknown op");
    optr[oo] = r;
  });
  return out;
}

pluss::codem::Series impl_dt_field(const Timeline& a, std::string_view field) {
  check_timeline_core(a.ticks_);
  const auto tz = TimeZone::load(a.tz());
  // We'll return Int32 for most fields, Int64 for nanos.
  const bool want_i64 = (field == "nanosecond");
  auto out = pluss::codem::Series::empty(a.ticks_.sizes(), want_i64 ? pluss::codem::ScalarType::Int64 : pluss::codem::ScalarType::Int32);
  auto it = pluss::codem::SeriesIterator::unary(a.ticks_, out);
  const auto* ap = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  if (want_i64) {
    auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
    it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
      const auto v = ap[ao];
      if (v == Timeline::kNaT) { op[oo] = -1; return; }
      int y, mo, d, hh, mm, ss, nsec;
      epoch_nanos_to_local_fields(v, tz, y, mo, d, hh, mm, ss, nsec);
      if (field == "nanosecond") op[oo] = nsec;
      else op[oo] = -1;
    });
    return out;
  }

  auto* op = reinterpret_cast<std::int32_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    const auto v = ap[ao];
    if (v == Timeline::kNaT) { op[oo] = -1; return; }
    int y, mo, d, hh, mm, ss, nsec;
    epoch_nanos_to_local_fields(v, tz, y, mo, d, hh, mm, ss, nsec);

    if (field == "year") op[oo] = (std::int32_t)y;
    else if (field == "month") op[oo] = (std::int32_t)mo;
    else if (field == "day") op[oo] = (std::int32_t)d;
    else if (field == "hour") op[oo] = (std::int32_t)hh;
    else if (field == "minute") op[oo] = (std::int32_t)mm;
    else if (field == "second") op[oo] = (std::int32_t)ss;
    else if (field == "microsecond") op[oo] = (std::int32_t)(nsec / 1000);
    else if (field == "millisecond") op[oo] = (std::int32_t)(nsec / 1000000);
    else if (field == "weekday") {
      const auto dp = sys_days(year(y)/month((unsigned)mo)/day((unsigned)d));
      int wd = (int)weekday(dp).c_encoding(); // Sun=0
      int mon0 = (wd + 6) % 7;
      op[oo] = (std::int32_t)mon0;
    } else if (field == "dayofyear") {
      const auto dp = sys_days(year(y)/month((unsigned)mo)/day((unsigned)d));
      const auto jan1 = sys_days(year(y)/month(1)/day(1));
      op[oo] = (std::int32_t)((dp - jan1).count() + 1);
    } else if (field == "quarter") {
      op[oo] = (std::int32_t)((mo - 1) / 3 + 1);
    } else if (field == "week") {
      // ISO week number (approx using chrono weekday + weeknum algorithm)
      // Use ISO year/week via year_month_day + weekday and a known approach.
      const auto dp = sys_days(year(y)/month((unsigned)mo)/day((unsigned)d));
      const auto wd = weekday(dp);
      const auto thursday = dp + days{3} - days{(wd.iso_encoding() + 6) % 7};
      const auto iso_year = int(year_month_day(thursday).year());
      const auto first_thu = sys_days(year(iso_year)/month(1)/day(4));
      const auto week1 = first_thu + days{3} - days{(weekday(first_thu).iso_encoding() + 6) % 7};
      op[oo] = (std::int32_t)((thursday - week1).count() / 7 + 1);
    } else {
      CODEM_FAIL("Timeline.dt: unsupported field");
    }
  });
  return out;
}

pluss::codem::Series impl_dt_predicate(const Timeline& a, std::string_view pred_op) {
  check_timeline_core(a.ticks_);
  const auto tz = TimeZone::load(a.tz());
  auto out = pluss::codem::Series::empty(a.ticks_.sizes(), pluss::codem::ScalarType::Bool);
  auto it = pluss::codem::SeriesIterator::unary(a.ticks_, out);
  const auto* ap = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  auto* op = reinterpret_cast<bool*>(out.raw_data());

  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    const auto v = ap[ao];
    if (v == Timeline::kNaT) { op[oo] = false; return; }
    int y, mo, d, hh, mm, ss, nsec;
    epoch_nanos_to_local_fields(v, tz, y, mo, d, hh, mm, ss, nsec);

    const auto dp = sys_days(year(y)/month((unsigned)mo)/day((unsigned)d));
    const int wd_sun0 = (int)weekday(dp).c_encoding();
    const int wd_mon0 = (wd_sun0 + 6) % 7;

    bool r = false;
    if (pred_op == "is_leap_year") r = year(y).is_leap();
    else if (pred_op == "is_month_start") r = (d == 1);
    else if (pred_op == "is_month_end") {
      const auto ymd = year_month_day(dp);
      r = (ymd.day() == (ymd.year()/ymd.month()/last).day());
    } else if (pred_op == "is_quarter_start") r = (d == 1) && ((mo == 1) || (mo == 4) || (mo == 7) || (mo == 10));
    else if (pred_op == "is_quarter_end") {
      bool end_month = (mo == 3 || mo == 6 || mo == 9 || mo == 12);
      if (end_month) {
        const auto ymd = year_month_day(dp);
        r = (ymd.day() == (ymd.year()/ymd.month()/last).day());
      }
    } else if (pred_op == "is_year_start") r = (mo == 1 && d == 1);
    else if (pred_op == "is_year_end") r = (mo == 12) && (year_month_day(dp).day() == (year(y)/month(12)/last).day());
    else if (pred_op == "is_weekend") r = (wd_mon0 == 5 || wd_mon0 == 6);
    else if (pred_op == "is_business_day") r = !(wd_mon0 == 5 || wd_mon0 == 6);
    else if (pred_op == "is_monday") r = (wd_mon0 == 0);
    else if (pred_op == "is_tuesday") r = (wd_mon0 == 1);
    else if (pred_op == "is_wednesday") r = (wd_mon0 == 2);
    else if (pred_op == "is_thursday") r = (wd_mon0 == 3);
    else if (pred_op == "is_friday") r = (wd_mon0 == 4);
    else if (pred_op == "is_saturday") r = (wd_mon0 == 5);
    else if (pred_op == "is_sunday") r = (wd_mon0 == 6);
    else CODEM_FAIL("Timeline predicate: unsupported predicate");
    op[oo] = r;
  });

  return out;
}

Timeline impl_dt_round(const Timeline& a, std::string_view mode, std::string_view unit) {
  check_timeline_core(a.ticks_);
  const auto tz = TimeZone::load(a.tz());
  const auto q = unit_nanos(unit);
  auto out = pluss::codem::Series::empty(a.ticks_.sizes(), pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::unary(a.ticks_, out);
  const auto* ap = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());

  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    const auto v = ap[ao];
    if (v == Timeline::kNaT) { op[oo] = Timeline::kNaT; return; }
    const int off = tz.offset_minutes_at_utc(v);
    const __int128 local128 = static_cast<__int128>(v) +
        static_cast<__int128>(off) * 60 * 1'000'000'000LL;
    CODEM_CHECK(local128 > std::numeric_limits<std::int64_t>::min() &&
                    local128 <= std::numeric_limits<std::int64_t>::max(),
                "Timeline round: local timestamp is outside the supported range");
    const auto local = static_cast<std::int64_t>(local128);

    // C++ division truncates toward zero. Build a mathematical floor quotient
    // so pre-epoch timestamps obey the same boundary rules as later values.
    std::int64_t quotient = local / q;
    std::int64_t remainder = local % q;
    if (remainder < 0) {
      remainder += q;
      --quotient;
    }
    __int128 rounded = static_cast<__int128>(quotient) * q;
    if (mode == "floor") {
      // `rounded` is already the lower boundary.
    } else if (mode == "ceil") {
      if (remainder != 0) rounded += q;
    } else if (mode == "round") {
      const auto twice = static_cast<__int128>(remainder) * 2;
      if (twice > q || (twice == q && (quotient % 2 != 0))) rounded += q;
    } else {
      CODEM_FAIL("Timeline round: unknown mode");
    }
    CODEM_CHECK(rounded > std::numeric_limits<std::int64_t>::min() &&
                    rounded <= std::numeric_limits<std::int64_t>::max(),
                "Timeline round: result is outside the supported range");
    const auto local_rounded = static_cast<std::int64_t>(rounded);

    const auto earlier = tz.local_to_utc(local_rounded, 0, "raise");
    const auto later = tz.local_to_utc(local_rounded, 1, "raise");
    if (earlier != later && tz.offset_minutes_at_utc(later) == off) {
      op[oo] = later;
    } else {
      op[oo] = earlier;
    }
  });

  return Timeline(std::move(out), std::string(a.tz()));
}

std::string impl_isoformat(const Timeline& a) {
  check_timeline_core(a.ticks_);
  CODEM_CHECK(a.dim() == 0, "Timeline::isoformat is scalar-only (0D)");
  const auto v = reinterpret_cast<const std::int64_t*>(a.ticks_.raw_data())[0];
  const auto tz = TimeZone::load(a.tz());
  return strftime_one(v, tz, "%Y-%m-%dT%H:%M:%S.%f%z");
}

pluss::coden::Note impl_strftime(const Timeline& a, std::string_view fmt) {
  return impl_format(a, fmt, {}, "NaT", 9, false);
}

pluss::coden::Note impl_format(const Timeline& a, std::string_view fmt,
                               std::string_view timezone, std::string_view nat_text,
                               int fractional_digits, bool trim_fractional_zeros) {
  check_timeline_core(a.ticks_);
  CODEM_CHECK(fractional_digits >= 1 && fractional_digits <= 9,
              "pluss::timeline::format: fractional_digits must be in [1, 9]");
  const std::string tz_name = timezone.empty() ? std::string(a.tz()) : std::string(timezone);
  const auto tz = TimeZone::load(tz_name);
  const auto pieces = compile_format(fmt);
  const auto n = a.numel();
  std::vector<std::string> storage;
  storage.reserve((size_t)n);
  std::vector<std::string_view> views;
  views.reserve((size_t)n);

  const auto ticks = a.ticks();
  for (std::int64_t i = 0; i < n; ++i) {
    storage.push_back(format_one(ticks[static_cast<std::size_t>(i)], tz, tz_name,
                                 pieces, nat_text, fractional_digits,
                                 trim_fractional_zeros));
  }
  for (auto& s : storage) views.push_back(s);

  return pluss::make_note(views, a.ticks_.sizes());
}

Timeline impl_date_range(const Timeline& start, const Timeline& end, std::string_view freq, std::int64_t step) {
  check_timeline_core(start.ticks_);
  check_timeline_core(end.ticks_);
  CODEM_CHECK(start.dim() == 0 && end.dim() == 0, "date_range requires scalar start/end");
  CODEM_CHECK(step != 0, "date_range step must be non-zero");
  const auto s = reinterpret_cast<const std::int64_t*>(start.ticks_.raw_data())[0];
  const auto e = reinterpret_cast<const std::int64_t*>(end.ticks_.raw_data())[0];
  CODEM_CHECK(s != Timeline::kNaT && e != Timeline::kNaT, "date_range does not support NaT endpoints");

  const bool calendar_days = is_day_unit(freq);
  const auto q = unit_nanos(freq);
  CODEM_CHECK(calendar_days ||
                  (step <= std::numeric_limits<std::int64_t>::max() / q &&
                   step >= std::numeric_limits<std::int64_t>::min() / q),
              "date_range step overflows nanoseconds");
  const auto delta = calendar_days ? step : q * step;
  const auto tz = TimeZone::load(start.tz());

  auto advance = [&](std::int64_t tick) {
    if (!calendar_days) return checked_tick_add(tick, delta, "date_range");

    constexpr __int128 ns_per_second = 1'000'000'000LL;
    constexpr __int128 ns_per_day = 86'400LL * ns_per_second;
    const int offset = tz.offset_minutes_at_utc(tick);
    const __int128 local_value = static_cast<__int128>(tick) +
        static_cast<__int128>(offset) * 60 * ns_per_second;
    CODEM_CHECK(local_value > std::numeric_limits<std::int64_t>::min() &&
                    local_value <= std::numeric_limits<std::int64_t>::max(),
                "date_range local timestamp is outside the supported range");

    const auto local = sys_time<nanoseconds>(nanoseconds(static_cast<std::int64_t>(local_value)));
    const auto local_day = floor<days>(local);
    const auto time_of_day = local - local_day;
    const __int128 target_day = static_cast<__int128>(local_day.time_since_epoch().count()) + step;
    CODEM_CHECK(target_day >= std::numeric_limits<std::int64_t>::min() &&
                    target_day <= std::numeric_limits<std::int64_t>::max(),
                "date_range day step is outside the supported range");
    const __int128 target_local = target_day * ns_per_day + time_of_day.count();
    constexpr __int128 offset_margin = 24 * 60 * 60 * ns_per_second;
    CODEM_CHECK(target_local >
                    static_cast<__int128>(std::numeric_limits<std::int64_t>::min()) + offset_margin &&
                    target_local <=
                    static_cast<__int128>(std::numeric_limits<std::int64_t>::max()) - offset_margin,
                "date_range result is outside the supported range");
    return tz.local_to_utc(static_cast<std::int64_t>(target_local), 0, "raise");
  };

  std::vector<std::int64_t> out;
  if (step > 0) {
    for (std::int64_t v = s; v <= e;) {
      out.push_back(v);
      if (v == e) break;
      const auto next = advance(v);
      if (next > e) break;
      v = next;
    }
  } else {
    for (std::int64_t v = s; v >= e;) {
      out.push_back(v);
      if (v == e) break;
      const auto next = advance(v);
      if (next < e) break;
      v = next;
    }
  }
  const std::array<std::int64_t, 1> shape{
      static_cast<std::int64_t>(out.size())};
  return Timeline(ops_internal::timeline_from_epoch_nanos(out, shape).ticks_.clone(),
                  std::string(start.tz()));
}

Timeline impl_tz_convert(const Timeline& a, std::string_view tz_name) {
  check_timeline_core(a.ticks_);
  (void)TimeZone::load(tz_name); // validate
  return Timeline(a.ticks_.clone(), std::string(tz_name));
}

Timeline impl_tz_localize(const Timeline& a, std::string_view tz_name, int fold, std::string_view nonexistent) {
  // Interpret ticks_ as naive *local* wall times in tz_name, convert to UTC instants.
  // This implementation supports fixed offsets directly; transition tables are handled by TimeZone registry.
  check_timeline_core(a.ticks_);
  const auto tz = TimeZone::load(tz_name);

  auto out = pluss::codem::Series::empty(a.ticks_.sizes(), pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::unary(a.ticks_, out);
  const auto* ap = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());

  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    const auto local_ns = ap[ao];
    if (local_ns == Timeline::kNaT) { op[oo] = Timeline::kNaT; return; }
    op[oo] = tz.local_to_utc(local_ns, fold, nonexistent);
  });

  return impl_tz_convert(Timeline(std::move(out), "UTC"), tz_name);
}

Timeline impl_time_of_day(const Timeline& a) {
  // Return a Timeline where date is epoch day 1970-01-01 but time preserved in local tz.
  check_timeline_core(a.ticks_);
  const auto tz = TimeZone::load(a.tz());
  auto out = pluss::codem::Series::empty(a.ticks_.sizes(), pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::unary(a.ticks_, out);
  const auto* ap = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  const auto base = sys_days(year(1970)/month(1)/day(1)).time_since_epoch().count();
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    const auto v = ap[ao];
    if (v == Timeline::kNaT) { op[oo] = Timeline::kNaT; return; }
    int y, mo, d, hh, mm, ss, nsec;
    epoch_nanos_to_local_fields(v, tz, y, mo, d, hh, mm, ss, nsec);
    std::int64_t tod = (std::int64_t)hh * 3600LL*1'000'000'000LL + (std::int64_t)mm * 60LL*1'000'000'000LL + (std::int64_t)ss * 1'000'000'000LL + nsec;
    // Re-localize the clock fields on the anchor date; the source date's DST
    // offset need not be the offset in effect on 1970-01-01.
    op[oo] = tz.local_to_utc(base + tod, 0, "raise");
  });
  return Timeline(std::move(out), std::string(a.tz()));
}

pluss::codem::Series impl_isnat(const Timeline& a) {
  check_timeline_core(a.ticks_);
  auto out = pluss::codem::Series::empty(a.ticks_.sizes(), pluss::codem::ScalarType::Bool);
  auto it = pluss::codem::SeriesIterator::unary(a.ticks_, out);
  const auto* ap = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  auto* op = reinterpret_cast<bool*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    op[oo] = (ap[ao] == Timeline::kNaT);
  });
  return out;
}

pluss::codem::Series impl_isvalid(const Timeline& a) {
  check_timeline_core(a.ticks_);
  auto out = pluss::codem::Series::empty(a.ticks_.sizes(), pluss::codem::ScalarType::Bool);
  auto it = pluss::codem::SeriesIterator::unary(a.ticks_, out);
  const auto* ap = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  auto* op = reinterpret_cast<bool*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    op[oo] = (ap[ao] != Timeline::kNaT);
  });
  return out;
}

bool impl_isnat_scalar(const Timeline& a) {
  check_timeline_core(a.ticks_);
  CODEM_CHECK(a.dim() == 0, "isnat_scalar expects 0D");
  const auto v = reinterpret_cast<const std::int64_t*>(a.ticks_.raw_data())[0];
  return v == Timeline::kNaT;
}

pluss::codem::Series impl_argsort(const Timeline& a) {
  check_timeline_core(a.ticks_);
  CODEM_CHECK(a.dim() == 1, "argsort currently supports 1D only");
  const auto n = a.numel();
  if (n <= 1) {
    if (n == 0) {
      return make_i64_series({});
    }
    return make_i64_series({0});
  }

  const auto valid_mask = impl_isvalid(a)
                              .to(pluss::codem::Platform::CPU)
                              .to_vec<bool>();

  std::vector<std::int64_t> valid_idx;
  std::vector<std::int64_t> nat_idx;
  valid_idx.reserve(static_cast<std::size_t>(n));
  nat_idx.reserve(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    if (valid_mask[static_cast<std::size_t>(i)]) {
      valid_idx.push_back(i);
    } else {
      nat_idx.push_back(i);
    }
  }

  std::vector<std::int64_t> out_idx;
  out_idx.reserve(static_cast<std::size_t>(n));
  if (!valid_idx.empty()) {
    auto valid_idx_series = make_i64_series(valid_idx);
    if (valid_idx_series.platform() != a.ticks_.platform()) {
      valid_idx_series = valid_idx_series.to(a.ticks_.platform());
    }
    const auto valid_ticks = a.ticks_.take(valid_idx_series);
    const auto local_order = valid_ticks
        .argsort(/*dim=*/0, /*descending=*/false, /*stable=*/true)
        .to(pluss::codem::Platform::CPU)
        .to_vec<std::int64_t>();

    for (const auto li : local_order) {
      out_idx.push_back(valid_idx[static_cast<std::size_t>(li)]);
    }
  }
  out_idx.insert(out_idx.end(), nat_idx.begin(), nat_idx.end());
  return make_i64_series(out_idx);
}

Timeline impl_min(const Timeline& a, bool skipna) {
  check_timeline_core(a.ticks_);
  const auto values = a.ticks();
  std::int64_t best = Timeline::kNaT;
  for (const auto v : values) {
    if (v == Timeline::kNaT) {
      if (!skipna) {
        best = Timeline::kNaT;
        break;
      }
      continue;
    }
    if (best == Timeline::kNaT || v < best) best = v;
  }
  return impl_tz_convert(
      ops_internal::timeline_from_epoch_nanos(
          std::span<const std::int64_t>(&best, 1), {}),
      a.tz());
}

Timeline impl_max(const Timeline& a, bool skipna) {
  check_timeline_core(a.ticks_);
  const auto values = a.ticks();
  std::int64_t best = Timeline::kNaT;
  for (const auto v : values) {
    if (v == Timeline::kNaT) {
      if (!skipna) {
        best = Timeline::kNaT;
        break;
      }
      continue;
    }
    if (best == Timeline::kNaT || v > best) best = v;
  }
  return impl_tz_convert(
      ops_internal::timeline_from_epoch_nanos(
          std::span<const std::int64_t>(&best, 1), {}),
      a.tz());
}

Timeline impl_sort(const Timeline& a) {
  check_timeline_core(a.ticks_);
  CODEM_CHECK(a.dim() == 1, "sort currently supports 1D only");
  auto order = impl_argsort(a);
  if (order.platform() != a.ticks_.platform()) {
    order = order.to(a.ticks_.platform());
  }
  return Timeline(a.ticks_.take(order), std::string(a.tz()));
}

Timeline impl_unique(const Timeline& a) {
  check_timeline_core(a.ticks_);
  CODEM_CHECK(a.dim() == 1, "unique currently supports 1D only");
  const auto values = a.ticks();
  std::unordered_set<std::int64_t> seen;
  std::vector<std::int64_t> out;
  out.reserve(values.size());
  for (const auto value : values) {
    if (seen.insert(value).second) out.push_back(value);
  }
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(out.size())};
  return impl_tz_convert(ops_internal::timeline_from_epoch_nanos(out, shape),
                         a.tz());
}

Timeline impl_fill_nat(const Timeline& a, const Timeline& value) {
  check_timeline_core(a.ticks_);
  check_timeline_core(value.ticks_);
  CODEM_CHECK(value.dim() == 0, "fill_nat value must be scalar Timeline");
  const auto v = reinterpret_cast<const std::int64_t*>(value.ticks_.raw_data())[0];
  auto out = pluss::codem::Series::empty(a.ticks_.sizes(), pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::unary(a.ticks_, out);
  const auto* ap = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    op[oo] = (ap[ao] == Timeline::kNaT) ? v : ap[ao];
  });
  return Timeline(std::move(out), std::string(a.tz()));
}

Timeline impl_where(const Timeline& a, const pluss::codem::Series& mask, const Timeline& other) {
  check_timeline_core(a.ticks_);
  check_timeline_core(other.ticks_);
  CODEM_CHECK(mask.scalar_type() == pluss::codem::ScalarType::Bool, "where mask must be bool Series");
  CODEM_CHECK(std::string_view(a.tz()) == std::string_view(other.tz()),
              "Timeline::where: timezone mismatch");
  const auto shape = pluss::codem::SeriesIterator::broadcast_shape(a.ticks_.sizes(), other.ticks_.sizes());
  const auto shape2 = pluss::codem::SeriesIterator::broadcast_shape(shape, mask.sizes());
  auto out = pluss::codem::Series::empty(shape2, pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::ternary(mask, a.ticks_, other.ticks_, out);
  const auto* mp = reinterpret_cast<const bool*>(it.m().raw_data());
  const auto* ap = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  const auto* bp = reinterpret_cast<const std::int64_t*>(it.b().raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  it.for_each_index_ternary([&](std::int64_t, std::int64_t mo, std::int64_t ao, std::int64_t bo, std::int64_t oo) {
    op[oo] = mp[mo] ? ap[ao] : bp[bo];
  });
  return Timeline(std::move(out), std::string(a.tz()));
}

pluss::codem::Series impl_searchsorted(const Timeline& a, const Timeline& values, bool left) {
  check_timeline_core(a.ticks_);
  check_timeline_core(values.ticks_);
  CODEM_CHECK(a.dim() == 1, "searchsorted currently supports 1D sorted Timeline");
  const auto n = a.numel();
  const auto sorted = a.ticks();
  const auto* x = sorted.data();
  std::int64_t valid_end = n;
  while (valid_end > 0 && x[valid_end - 1] == Timeline::kNaT) --valid_end;
  auto out = pluss::codem::Series::empty(values.ticks_.sizes(), pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::unary(values.ticks_, out);
  const auto* vp = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    const auto k = vp[ao];
    if (k == Timeline::kNaT) {
      op[oo] = left ? valid_end : n;
      return;
    }
    if (left) op[oo] = (std::int64_t)(std::lower_bound(x, x+valid_end, k) - x);
    else op[oo] = (std::int64_t)(std::upper_bound(x, x+valid_end, k) - x);
  });
  return out;
}

Timeline impl_add_nanos(const Timeline& a, std::int64_t nanos) {
  check_timeline_core(a.ticks_);
  auto out = pluss::codem::Series::empty(a.ticks_.sizes(), pluss::codem::ScalarType::Int64);
  apply_unary_i64_scalar(a.ticks_, out, [nanos](std::int64_t v) -> std::int64_t {
    return (v == Timeline::kNaT)
        ? Timeline::kNaT
        : checked_tick_add(v, nanos, "Timeline::add_nanos");
  });
  return Timeline(std::move(out), std::string(a.tz()));
}

pluss::coden::Note impl_weekday_name(const Timeline& a) {
  return impl_strftime(a, "%A");
}

pluss::coden::Note impl_month_name(const Timeline& a) {
  return impl_strftime(a, "%B");
}

} // namespace detail

namespace {

static const Timeline& as_timeline(const TimelineBase& b) {
  return static_cast<const Timeline&>(b);
}

static std::int64_t scalar_i64_series(const pluss::codem::Series& s, std::string_view where) {
  CODEM_CHECK(s.defined(), std::string(where) + ": undefined Series");
  CODEM_CHECK(s.dim() == 0, std::string(where) + ": scalar-only");
  const auto c = (s.scalar_type() == pluss::codem::ScalarType::Int64)
      ? s
      : s.to(pluss::codem::ScalarType::Int64);
  return reinterpret_cast<const std::int64_t*>(c.raw_data())[0];
}

static bool scalar_bool_series(const pluss::codem::Series& s, std::string_view where) {
  CODEM_CHECK(s.defined(), std::string(where) + ": undefined Series");
  CODEM_CHECK(s.scalar_type() == pluss::codem::ScalarType::Bool,
              std::string(where) + ": expected bool Series");
  CODEM_CHECK(s.dim() == 0, std::string(where) + ": scalar-only");
  return reinterpret_cast<const bool*>(s.raw_data())[0];
}

static std::string scalar_note(const pluss::coden::Note& n, std::string_view where) {
  CODEM_CHECK(n.defined(), std::string(where) + ": undefined Note");
  CODEM_CHECK(n.dim() == 0 && n.numel() == 1, std::string(where) + ": scalar-only");
  return std::string(n.at(0));
}

static int scalar_field(const Timeline& t, std::string_view field) {
  const auto v = scalar_i64_series(detail::impl_dt_field(t, field), "Timeline scalar field");
  return static_cast<int>(v);
}

static bool scalar_pred(const Timeline& t, std::string_view pred) {
  return scalar_bool_series(detail::impl_dt_predicate(t, pred), "Timeline scalar predicate");
}

static std::int64_t scalar_tick_utc(const Timeline& t) {
  CODEM_CHECK(t.dim() == 0, "Timeline: scalar-only");
  return reinterpret_cast<const std::int64_t*>(t.unsafe_ticks().raw_data())[0];
}

static std::int64_t epoch_day_ymd_(int y, unsigned mo, unsigned d) {
  return static_cast<std::int64_t>(
      (sys_days{year{y} / month{mo} / day{d}}).time_since_epoch().count());
}

static int weekday_sun0_from_epoch_day_(std::int64_t day_label) {
  const sys_days dp{days(day_label)};
  return static_cast<int>(weekday(dp).c_encoding());
}

static int weekday_sun0_from_ymd_(int y, unsigned mo, unsigned d) {
  return static_cast<int>(weekday(sys_days{year{y} / month{mo} / day{d}}).c_encoding());
}

static unsigned days_in_month_raw_(int y, unsigned mo) {
  const auto first = sys_days(year{y} / month{mo} / day{1});
  const auto next = (mo == 12) ? sys_days(year{y + 1} / month{1} / day{1})
                               : sys_days(year{y} / month{mo + 1} / day{1});
  return static_cast<unsigned>((next - first).count());
}

static std::int64_t observed_fixed_weekday_(int y, unsigned mo, unsigned d) {
  const auto actual = epoch_day_ymd_(y, mo, d);
  const int wd = weekday_sun0_from_ymd_(y, mo, d);
  if (wd == 6) return actual - 1; // Saturday -> Friday
  if (wd == 0) return actual + 1; // Sunday -> Monday
  return actual;
}

static std::int64_t observed_fixed_forward_(int y, unsigned mo, unsigned d) {
  auto out = epoch_day_ymd_(y, mo, d);
  int wd = weekday_sun0_from_ymd_(y, mo, d);
  while (wd == 0 || wd == 6) {
    ++out;
    wd = (wd + 1) % 7;
  }
  return out;
}

static std::int64_t nth_weekday_(int y, unsigned mo, int weekday_sun0, int nth) {
  CODEM_CHECK(nth >= 1 && nth <= 5, "BusinessCalendar: nth weekday must be 1..5");
  const int first_wd = weekday_sun0_from_ymd_(y, mo, 1);
  const unsigned first = 1U + static_cast<unsigned>((weekday_sun0 - first_wd + 7) % 7);
  const unsigned day_value = first + static_cast<unsigned>((nth - 1) * 7);
  CODEM_CHECK(day_value <= days_in_month_raw_(y, mo), "BusinessCalendar: nth weekday does not exist");
  return epoch_day_ymd_(y, mo, day_value);
}

static std::int64_t last_weekday_(int y, unsigned mo, int weekday_sun0) {
  const unsigned last_day = days_in_month_raw_(y, mo);
  const int last_wd = weekday_sun0_from_ymd_(y, mo, last_day);
  const unsigned day_value = last_day - static_cast<unsigned>((last_wd - weekday_sun0 + 7) % 7);
  return epoch_day_ymd_(y, mo, day_value);
}

static sys_days western_easter_(int y) {
  // Meeus/Jones/Butcher Gregorian computus.
  const int a = y % 19;
  const int b = y / 100;
  const int c = y % 100;
  const int d = b / 4;
  const int e = b % 4;
  const int f = (b + 8) / 25;
  const int g = (b - f + 1) / 3;
  const int h = (19 * a + b - d - g + 15) % 30;
  const int i = c / 4;
  const int k = c % 4;
  const int l = (32 + 2 * e + 2 * i - h - k) % 7;
  const int m = (a + 11 * h + 22 * l) / 451;
  const int month_value = (h + l - 7 * m + 114) / 31;
  const int day_value = ((h + l - 7 * m + 114) % 31) + 1;
  return sys_days{year{y} / month{static_cast<unsigned>(month_value)} /
                  day{static_cast<unsigned>(day_value)}};
}

static std::int64_t western_easter_offset_(int y, int offset_days) {
  return static_cast<std::int64_t>((western_easter_(y) + days{offset_days}).time_since_epoch().count());
}

static void add_holiday_(std::set<std::int64_t>& out, std::int64_t day_label) {
  out.insert(day_label);
}

static void add_observed_fixed_(std::set<std::int64_t>& out, int y, unsigned mo, unsigned d) {
  add_holiday_(out, observed_fixed_weekday_(y, mo, d));
}

static void add_observed_fixed_forward_(std::set<std::int64_t>& out, int y, unsigned mo, unsigned d) {
  add_holiday_(out, observed_fixed_forward_(y, mo, d));
}

static void add_christmas_boxing_forward_(std::set<std::int64_t>& out, int y) {
  std::set<std::int64_t> used;
  const auto christmas = epoch_day_ymd_(y, 12, 25);
  const auto boxing = epoch_day_ymd_(y, 12, 26);
  auto observed_christmas = christmas;
  while (weekday_sun0_from_epoch_day_(observed_christmas) == 0 ||
         weekday_sun0_from_epoch_day_(observed_christmas) == 6 ||
         used.contains(observed_christmas)) {
    ++observed_christmas;
  }
  used.insert(observed_christmas);
  add_holiday_(out, observed_christmas);
  auto observed_boxing = boxing;
  while (weekday_sun0_from_epoch_day_(observed_boxing) == 0 ||
         weekday_sun0_from_epoch_day_(observed_boxing) == 6 ||
         used.contains(observed_boxing)) {
    ++observed_boxing;
  }
  add_holiday_(out, observed_boxing);
}

static void add_unique_holidays_(BusinessCalendar& calendar, const std::set<std::int64_t>& days) {
  calendar.holidays.assign(days.begin(), days.end());
}

static void check_calendar_year_range_(int year_start, int year_end, std::string_view where) {
  CODEM_CHECK(year_start >= 1900 && year_start <= 2200,
              std::string(where) + ": year_start must be in [1900, 2200]");
  CODEM_CHECK(year_end >= 1900 && year_end <= 2200,
              std::string(where) + ": year_end must be in [1900, 2200]");
  CODEM_CHECK(year_start <= year_end,
              std::string(where) + ": year_start must be <= year_end");
}

static Timeline scalar_from_utc_ns(std::int64_t ns_utc, std::string_view tz_name) {
  const std::array<std::int64_t, 1> flat{ns_utc};
  Timeline out = ops_internal::timeline_from_epoch_nanos(flat, {});
  if (tz_name != "UTC") {
    out = detail::impl_tz_convert(out, tz_name);
  }
  return out;
}

static std::string lower_ascii(std::string_view sv) {
  std::string out(sv.begin(), sv.end());
  std::transform(out.begin(), out.end(), out.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return out;
}

static unsigned days_in_month_ym(int y, unsigned mo) {
  const auto first = sys_days(year{y} / month{mo} / day{1});
  const auto next = (mo == 12) ? sys_days(year{y + 1} / month{1} / day{1})
                               : sys_days(year{y} / month{mo + 1} / day{1});
  return static_cast<unsigned>((next - first).count());
}

struct ComponentInput {
  std::vector<std::int64_t> flat;
  std::vector<std::int64_t> shape;
  bool scalar{false};
};

static std::vector<std::int64_t> shape_or_scalar(const pluss::codem::Series& s,
                                                 std::string_view where) {
  CODEM_CHECK(s.defined(), std::string(where) + ": undefined Series");
  CODEM_CHECK(s.dim() >= 0 && s.dim() <= 3, std::string(where) + ": supports 0D..3D");
  return std::vector<std::int64_t>(s.sizes().begin(), s.sizes().end());
}

static ComponentInput load_component_i64(const pluss::codem::Series& s,
                                         std::string_view name) {
  ComponentInput out;
  out.shape = shape_or_scalar(s, name);
  out.scalar = out.shape.empty();
  const auto i64 = (s.scalar_type() == pluss::codem::ScalarType::Int64)
      ? s
      : s.to(pluss::codem::ScalarType::Int64);
  const auto c = i64.contiguous();
  out.flat.resize(static_cast<std::size_t>(c.numel()));
  if (!out.flat.empty()) {
    std::memcpy(out.flat.data(), c.raw_data(), out.flat.size() * sizeof(std::int64_t));
  }
  return out;
}

static std::vector<std::int64_t> broadcast_or_same_shape(const std::vector<ComponentInput>& xs,
                                                         std::string_view where) {
  std::vector<std::int64_t> shape;
  for (const auto& x : xs) {
    if (x.scalar) {
      continue;
    }
    if (shape.empty()) {
      shape = x.shape;
      continue;
    }
    CODEM_CHECK(shape == x.shape, std::string(where) + ": non-scalar components must share shape");
  }
  return shape;
}

static std::int64_t pick_component(const ComponentInput& x, std::int64_t idx) {
  return x.scalar ? x.flat[0] : x.flat[static_cast<std::size_t>(idx)];
}

static std::vector<std::int64_t> copy_epoch_scaled(const Timeline& t, std::int64_t denom) {
  auto ns = t.ticks();
  if (denom == 1) {
    return ns;
  }
  for (auto& v : ns) {
    if (v != Timeline::kNaT) {
      const auto remainder = v % denom;
      v /= denom;
      if (remainder < 0) --v;
    }
  }
  return ns;
}

static std::int64_t checked_epoch_scale(std::int64_t value,
                                        std::int64_t factor,
                                        std::string_view where) {
  CODEM_CHECK(value >= std::numeric_limits<std::int64_t>::min() / factor &&
                  value <= std::numeric_limits<std::int64_t>::max() / factor,
              std::string(where) + ": epoch value is outside nanosecond range");
  const auto scaled = value * factor;
  CODEM_CHECK(scaled != Timeline::kNaT,
              std::string(where) + ": epoch value collides with NaT");
  return scaled;
}

static pluss::codem::Series make_i64_series(std::span<const std::int64_t> flat,
                                            std::span<const std::int64_t> shape) {
  auto out = pluss::codem::Series::empty(shape, pluss::codem::ScalarType::Int64);
  if (!flat.empty()) {
    std::memcpy(out.raw_data(), flat.data(), flat.size() * sizeof(std::int64_t));
  }
  return out;
}

enum class LocalField { Year, Month, Day, Hour, Minute, Second };

static Timeline replace_local_field(const Timeline& self, LocalField field,
                                    int replacement, std::string_view where) {
  switch (field) {
    case LocalField::Year:
      CODEM_CHECK(year{replacement}.ok(),
                  std::string(where) + ": year is outside the supported range");
      break;
    case LocalField::Month:
      CODEM_CHECK(replacement >= 1 && replacement <= 12,
                  std::string(where) + ": month must be in [1, 12]");
      break;
    case LocalField::Day:
      CODEM_CHECK(replacement >= 1 && replacement <= 31,
                  std::string(where) + ": day must be in [1, 31]");
      break;
    case LocalField::Hour:
      CODEM_CHECK(replacement >= 0 && replacement <= 23,
                  std::string(where) + ": hour must be in [0, 23]");
      break;
    case LocalField::Minute:
      CODEM_CHECK(replacement >= 0 && replacement <= 59,
                  std::string(where) + ": minute must be in [0, 59]");
      break;
    case LocalField::Second:
      CODEM_CHECK(replacement >= 0 && replacement <= 59,
                  std::string(where) + ": second must be in [0, 59]");
      break;
  }

  const auto tz = TimeZone::load(self.tz());
  auto out = pluss::codem::Series::empty(self.sizes(), pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::unary(self.unsafe_ticks(), out);
  const auto* input = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  auto* output = reinterpret_cast<std::int64_t*>(out.raw_data());

  it.for_each_index([&](std::int64_t logical, std::int64_t input_offset,
                        std::int64_t, std::int64_t output_offset) {
    const auto tick = input[input_offset];
    if (tick == Timeline::kNaT) {
      output[output_offset] = Timeline::kNaT;
      return;
    }

    int y, mo, d, hh, mm, ss, nsec;
    epoch_nanos_to_local_fields(tick, tz, y, mo, d, hh, mm, ss, nsec);
    switch (field) {
      case LocalField::Year: y = replacement; break;
      case LocalField::Month: mo = replacement; break;
      case LocalField::Day: d = replacement; break;
      case LocalField::Hour: hh = replacement; break;
      case LocalField::Minute: mm = replacement; break;
      case LocalField::Second: ss = replacement; break;
    }

    const auto ymd = year{y} / month{static_cast<unsigned>(mo)} /
                     day{static_cast<unsigned>(d)};
    CODEM_CHECK(ymd.ok(),
                std::string(where) + ": invalid calendar date at logical position " +
                    std::to_string(logical));
    const auto local = sys_days(ymd) + hours(hh) + minutes(mm) + seconds(ss) +
                       nanoseconds(nsec);
    output[output_offset] = tz.local_to_utc(
        duration_cast<nanoseconds>(local.time_since_epoch()).count(), 0, "raise");
  });
  return pluss::codeh::ops_internal::timeline_from_series(
      std::move(out), std::string(self.tz()));
}

} // namespace

int TimelineBase::year_scalar() const { return scalar_field(as_timeline(*this), "year"); }
int TimelineBase::month_scalar() const { return scalar_field(as_timeline(*this), "month"); }
int TimelineBase::day_scalar() const { return scalar_field(as_timeline(*this), "day"); }
int TimelineBase::hour_scalar() const { return scalar_field(as_timeline(*this), "hour"); }
int TimelineBase::minute_scalar() const { return scalar_field(as_timeline(*this), "minute"); }
int TimelineBase::second_scalar() const { return scalar_field(as_timeline(*this), "second"); }
int TimelineBase::weekday_scalar() const { return scalar_field(as_timeline(*this), "weekday"); }
int TimelineBase::yearday_scalar() const { return scalar_field(as_timeline(*this), "dayofyear"); }
int TimelineBase::quarter_scalar() const { return scalar_field(as_timeline(*this), "quarter"); }
int TimelineBase::week_scalar() const { return scalar_field(as_timeline(*this), "week"); }

bool TimelineBase::leap_scalar() const { return scalar_pred(as_timeline(*this), "is_leap_year"); }
bool TimelineBase::month_start_scalar() const { return scalar_pred(as_timeline(*this), "is_month_start"); }
bool TimelineBase::month_end_scalar() const { return scalar_pred(as_timeline(*this), "is_month_end"); }
bool TimelineBase::quarter_start_scalar() const { return scalar_pred(as_timeline(*this), "is_quarter_start"); }
bool TimelineBase::quarter_end_scalar() const { return scalar_pred(as_timeline(*this), "is_quarter_end"); }
bool TimelineBase::is_weekend_scalar() const { return scalar_pred(as_timeline(*this), "is_weekend"); }

std::string TimelineBase::weekday_name(bool abbreviated) const {
  return format(abbreviated ? "%a" : "%A");
}

std::string TimelineBase::month_name(bool abbreviated) const {
  return format(abbreviated ? "%b" : "%B");
}

Timeline TimelineBase::set_year(int year) const {
  return replace_local_field(as_timeline(*this), LocalField::Year, year,
                             "Timeline::set_year");
}

Timeline TimelineBase::set_month(int month) const {
  return replace_local_field(as_timeline(*this), LocalField::Month, month,
                             "Timeline::set_month");
}

Timeline TimelineBase::set_day(int day) const {
  return replace_local_field(as_timeline(*this), LocalField::Day, day,
                             "Timeline::set_day");
}

Timeline TimelineBase::set_hour(int hour) const {
  return replace_local_field(as_timeline(*this), LocalField::Hour, hour,
                             "Timeline::set_hour");
}

Timeline TimelineBase::set_minute(int minute) const {
  return replace_local_field(as_timeline(*this), LocalField::Minute, minute,
                             "Timeline::set_minute");
}

Timeline TimelineBase::set_second(int second) const {
  return replace_local_field(as_timeline(*this), LocalField::Second, second,
                             "Timeline::set_second");
}

std::string TimelineBase::format(const std::string& format_str) const {
  return scalar_note(detail::impl_strftime(as_timeline(*this), format_str), "Timeline::format");
}

std::string TimelineBase::to_string() const { return format("%Y-%m-%d %H:%M:%S"); }

pluss::codem::Series TimelineBase::to_Series() const { return ticks_; }

Timeline TimelineBase::add_years(int years) const {
  return detail::impl_add_offset(as_timeline(*this), Years{years});
}

Timeline TimelineBase::add_months(int months) const {
  return detail::impl_add_offset(as_timeline(*this), Months{months});
}

Timeline TimelineBase::add_days(int days) const {
  return detail::impl_add_offset(as_timeline(*this), Days{days});
}

Timeline TimelineBase::add_hours(int hours_) const {
  constexpr std::int64_t scale = 3'600'000'000'000LL;
  CODEM_CHECK(hours_ <= std::numeric_limits<std::int64_t>::max() / scale &&
                  hours_ >= std::numeric_limits<std::int64_t>::min() / scale,
              "Timeline::add_hours: offset overflows nanoseconds");
  return detail::impl_add_nanos(as_timeline(*this),
                                static_cast<std::int64_t>(hours_) * scale);
}

Timeline TimelineBase::add_minutes(int minutes_) const {
  constexpr std::int64_t scale = 60'000'000'000LL;
  CODEM_CHECK(minutes_ <= std::numeric_limits<std::int64_t>::max() / scale &&
                  minutes_ >= std::numeric_limits<std::int64_t>::min() / scale,
              "Timeline::add_minutes: offset overflows nanoseconds");
  return detail::impl_add_nanos(as_timeline(*this),
                                static_cast<std::int64_t>(minutes_) * scale);
}

Timeline TimelineBase::add_seconds(int seconds_) const {
  constexpr std::int64_t scale = 1'000'000'000LL;
  return detail::impl_add_nanos(as_timeline(*this),
                                static_cast<std::int64_t>(seconds_) * scale);
}

namespace {

std::pair<std::int64_t, std::int64_t> between_ticks_(
    const Timeline& self,
    const Timeline& other,
    std::string_view where) {
  CODEM_CHECK(self.dim() == 0 && other.dim() == 0,
              std::string(where) + ": both operands must be scalar Timelines");
  const auto start = scalar_tick_utc(self);
  const auto end = scalar_tick_utc(other);
  CODEM_CHECK(start != Timeline::kNaT && end != Timeline::kNaT,
              std::string(where) + ": NaT has no distance");
  return {start, end};
}

std::int64_t elapsed_between_(
    const Timeline& self,
    const Timeline& other,
    std::int64_t nanos_per_unit,
    std::string_view where) {
  const auto [start, end] = between_ticks_(self, other, where);
  const __int128 delta = static_cast<__int128>(end) - start;
  const __int128 result = delta / nanos_per_unit;
  CODEM_CHECK(result >= std::numeric_limits<std::int64_t>::min() &&
                  result <= std::numeric_limits<std::int64_t>::max(),
              std::string(where) + ": result is outside the int64 range");
  return static_cast<std::int64_t>(result);
}

std::int64_t calendar_between_(
    const Timeline& self,
    const Timeline& other,
    bool years,
    std::string_view where) {
  const auto [start, end] = between_ticks_(self, other, where);
  CODEM_CHECK(self.tz() == other.tz(),
              std::string(where) +
                  ": calendar distances require matching timezones; convert first");
  if (start == end) {
    return 0;
  }

  const bool reversed = end < start;
  const Timeline& earlier = reversed ? other : self;
  const Timeline& later = reversed ? self : other;
  const auto earlier_year = static_cast<std::int64_t>(scalar_field(earlier, "year"));
  const auto later_year = static_cast<std::int64_t>(scalar_field(later, "year"));
  std::int64_t candidate = later_year - earlier_year;
  if (!years) {
    candidate = candidate * 12 + scalar_field(later, "month") -
                scalar_field(earlier, "month");
  }

  CODEM_CHECK(candidate >= 0 &&
                  candidate <= std::numeric_limits<int>::max(),
              std::string(where) + ": calendar distance is outside the supported range");
  const auto anniversary = years
      ? earlier.add_years(static_cast<int>(candidate))
      : earlier.add_months(static_cast<int>(candidate));
  if (scalar_tick_utc(anniversary) > scalar_tick_utc(later)) {
    --candidate;
  }
  return reversed ? -candidate : candidate;
}

} // namespace

std::int64_t TimelineBase::years_between(const Timeline& other) const {
  return calendar_between_(as_timeline(*this), other, true,
                           "Timeline::years_between");
}

std::int64_t TimelineBase::months_between(const Timeline& other) const {
  return calendar_between_(as_timeline(*this), other, false,
                           "Timeline::months_between");
}

std::int64_t TimelineBase::days_between(const Timeline& other) const {
  return elapsed_between_(as_timeline(*this), other, 86'400'000'000'000LL,
                          "Timeline::days_between");
}

std::int64_t TimelineBase::hours_between(const Timeline& other) const {
  return elapsed_between_(as_timeline(*this), other, 3'600'000'000'000LL,
                          "Timeline::hours_between");
}

std::int64_t TimelineBase::minutes_between(const Timeline& other) const {
  return elapsed_between_(as_timeline(*this), other, 60'000'000'000LL,
                          "Timeline::minutes_between");
}

std::int64_t TimelineBase::seconds_between(const Timeline& other) const {
  return elapsed_between_(as_timeline(*this), other, 1'000'000'000LL,
                          "Timeline::seconds_between");
}

namespace {

struct SpanFields {
  int year{1970};
  int month{1};
  int day{1};
  std::int64_t time_of_day_nanos{0};
};

SpanFields span_fields_(std::int64_t ns_utc, const TimeZone& tz) {
  int y = 1970;
  int mo = 1;
  int d = 1;
  int hh = 0;
  int mm = 0;
  int ss = 0;
  int nsec = 0;
  epoch_nanos_to_local_fields(ns_utc, tz, y, mo, d, hh, mm, ss, nsec);
  constexpr std::int64_t kNanosPerSecond = 1'000'000'000LL;
  return {y, mo, d,
          static_cast<std::int64_t>(hh * 3600 + mm * 60 + ss) *
                  kNanosPerSecond +
              nsec};
}

bool is_month_end_(const SpanFields& x) {
  return x.day == static_cast<int>(days_in_month_ym(x.year, static_cast<unsigned>(x.month)));
}

std::int64_t complete_months_forward_(
    const SpanFields& start,
    const SpanFields& end,
    TimelineSpanAnchor anchor) {
  std::int64_t months =
      static_cast<std::int64_t>(end.year - start.year) * 12LL +
      static_cast<std::int64_t>(end.month - start.month);
  if (end.day < start.day) {
    const bool month_end_equivalent =
        anchor == TimelineSpanAnchor::MonthEnd &&
        is_month_end_(start) &&
        is_month_end_(end);
    if (!month_end_equivalent) {
      --months;
    }
  }
  const bool equivalent_day =
      end.day == start.day ||
      (anchor == TimelineSpanAnchor::MonthEnd &&
       is_month_end_(start) && is_month_end_(end));
  if (equivalent_day && end.time_of_day_nanos < start.time_of_day_nanos) {
    --months;
  }
  return months;
}

std::int64_t complete_years_forward_(
    const SpanFields& start,
    const SpanFields& end,
    TimelineSpanAnchor anchor) {
  std::int64_t years = static_cast<std::int64_t>(end.year - start.year);
  if (end.month < start.month ||
      (end.month == start.month && end.day < start.day)) {
    const bool month_end_equivalent =
        anchor == TimelineSpanAnchor::MonthEnd &&
        start.month == 2 &&
        end.month == 2 &&
        is_month_end_(start) &&
        is_month_end_(end);
    if (!month_end_equivalent) {
      --years;
    }
  }
  const bool equivalent_anniversary =
      (end.month == start.month && end.day == start.day) ||
      (anchor == TimelineSpanAnchor::MonthEnd &&
       start.month == 2 && end.month == 2 &&
       is_month_end_(start) && is_month_end_(end));
  if (equivalent_anniversary &&
      end.time_of_day_nanos < start.time_of_day_nanos) {
    --years;
  }
  return years;
}

std::int64_t boundary_months_(const SpanFields& start, const SpanFields& end) {
  return static_cast<std::int64_t>(end.year - start.year) * 12LL +
         static_cast<std::int64_t>(end.month - start.month);
}

std::int64_t boundary_years_(const SpanFields& start, const SpanFields& end) {
  return static_cast<std::int64_t>(end.year - start.year);
}

std::int64_t local_epoch_day_(std::int64_t ns_utc, const TimeZone& tz) {
  const int off = tz.offset_minutes_at_utc(ns_utc);
  constexpr __int128 kNanosPerDay = 86'400LL * 1'000'000'000LL;
  const __int128 local_ns = static_cast<__int128>(ns_utc) +
      static_cast<__int128>(off) * 60LL * 1'000'000'000LL;
  __int128 day = local_ns / kNanosPerDay;
  if (local_ns < 0 && local_ns % kNanosPerDay != 0) {
    --day;
  }
  CODEM_CHECK(day >= std::numeric_limits<std::int64_t>::min() &&
                  day <= std::numeric_limits<std::int64_t>::max(),
              "Timeline::span: local day is outside the supported range");
  return static_cast<std::int64_t>(day);
}

int weekday_sun0_from_local_day_(std::int64_t local_day) {
  const sys_days dp{days(local_day)};
  return static_cast<int>(weekday(dp).c_encoding());
}

std::int64_t weekmask_popcount_(std::uint8_t weekmask) {
  std::int64_t count = 0;
  for (int wd = 0; wd < 7; ++wd) {
    if (((weekmask >> wd) & std::uint8_t{1}) != 0) {
      ++count;
    }
  }
  return count;
}

std::int64_t business_days_inclusive_(
    std::int64_t first,
    std::int64_t last,
    const BusinessCalendar& calendar,
    const std::set<std::int64_t>& holidays) {
  if (last < first) {
    return 0;
  }
  const std::int64_t n = last - first + 1;
  const std::int64_t full_weeks = n / 7;
  const std::int64_t rem = n % 7;
  std::int64_t count = full_weeks * weekmask_popcount_(calendar.weekmask);
  int wd = weekday_sun0_from_local_day_(first);
  for (std::int64_t i = 0; i < rem; ++i) {
    if (((calendar.weekmask >> wd) & std::uint8_t{1}) != 0) {
      ++count;
    }
    wd = (wd + 1) % 7;
  }
  for (auto it = holidays.lower_bound(first);
       it != holidays.end() && *it <= last;
       ++it) {
    const auto hwd = weekday_sun0_from_local_day_(*it);
    if (((calendar.weekmask >> hwd) & std::uint8_t{1}) != 0) {
      --count;
    }
  }
  return count;
}

std::int64_t business_days_forward_(
    std::int64_t start_ns,
    std::int64_t end_ns,
    const TimeZone& tz,
    const TimelineSpanOptions& options,
    const std::set<std::int64_t>& holidays) {
  const auto start_day = local_epoch_day_(start_ns, tz);
  const auto end_day = local_epoch_day_(end_ns, tz);
  const bool include_start =
      options.closed == TimelineSpanClosed::Left ||
      options.closed == TimelineSpanClosed::Both;
  const bool include_end =
      options.closed == TimelineSpanClosed::Right ||
      options.closed == TimelineSpanClosed::Both;
  std::int64_t first = start_day + (include_start ? 0 : 1);
  std::int64_t last = end_day - (include_end ? 0 : 1);
  if (last < first) {
    return 0;
  }
  return business_days_inclusive_(first, last, options.calendar, holidays);
}

void validate_span_options_(const Timeline& self,
                            const Timeline& other,
                            const TimelineSpanOptions& options) {
  const bool calendar_unit =
      options.unit == TimelineSpanUnit::Months ||
      options.unit == TimelineSpanUnit::Years ||
      options.unit == TimelineSpanUnit::BusinessDays;
  if (calendar_unit) {
    CODEM_CHECK(
        std::string_view(self.tz()) == std::string_view(other.tz()),
        "Timeline::span: calendar/business spans require matching timezones; convert first");
  }
  if (options.unit == TimelineSpanUnit::BusinessDays) {
    CODEM_CHECK((options.calendar.weekmask & 0b10000000) == 0,
                "Timeline::span(BusinessDays): BusinessCalendar.weekmask uses only 7 weekday bits");
    CODEM_CHECK((options.calendar.weekmask & 0b01111111) != 0,
                "Timeline::span(BusinessDays): BusinessCalendar.weekmask cannot be empty");
    if (!options.calendar.timezone.empty()) {
      CODEM_CHECK(
          std::string_view(options.calendar.timezone) == std::string_view(self.tz()),
          "Timeline::span(BusinessDays): BusinessCalendar.timezone must match Timeline timezone");
    }
    constexpr std::int64_t kMaxReasonableEpochDay = 20'000'000LL;
    for (const auto h : options.calendar.holidays) {
      CODEM_CHECK(h != Timeline::kNaT,
                  "Timeline::span(BusinessDays): holidays must be local epoch-day labels, not NaT");
      CODEM_CHECK(std::llabs(h) <= kMaxReasonableEpochDay,
                  "Timeline::span(BusinessDays): holiday value is suspicious; holidays must be local epoch-day labels, not epoch nanos");
    }
  }
}

std::int64_t timeline_span_one_(
    std::int64_t start_ns,
    std::int64_t end_ns,
    const TimeZone& tz,
    const TimelineSpanOptions& options,
    const std::set<std::int64_t>& holidays) {
  if (start_ns == Timeline::kNaT || end_ns == Timeline::kNaT) {
    return Timeline::kNaT;
  }

  constexpr std::int64_t kMicros = 1'000LL;
  constexpr std::int64_t kMillis = 1'000'000LL;
  constexpr std::int64_t kSeconds = 1'000'000'000LL;
  constexpr std::int64_t kMinutes = 60LL * kSeconds;
  constexpr std::int64_t kHours = 60LL * kMinutes;
  constexpr std::int64_t kDays = 86'400'000'000'000LL;
  constexpr std::int64_t kWeeks = 7LL * kDays;

  const auto fixed_span = [&](std::int64_t denom) {
    __int128 out = (static_cast<__int128>(end_ns) - start_ns) / denom;
    if (!options.signed_result && out < 0) {
      out = -out;
    }
    CODEM_CHECK(out > std::numeric_limits<std::int64_t>::min() &&
                    out <= std::numeric_limits<std::int64_t>::max(),
                "Timeline::span: result is outside the int64 range");
    return static_cast<std::int64_t>(out);
  };

  switch (options.unit) {
    case TimelineSpanUnit::Nanos:
      return fixed_span(1);
    case TimelineSpanUnit::Micros:
      return fixed_span(kMicros);
    case TimelineSpanUnit::Millis:
      return fixed_span(kMillis);
    case TimelineSpanUnit::Seconds:
      return fixed_span(kSeconds);
    case TimelineSpanUnit::Minutes:
      return fixed_span(kMinutes);
    case TimelineSpanUnit::Hours:
      return fixed_span(kHours);
    case TimelineSpanUnit::Days:
      return fixed_span(kDays);
    case TimelineSpanUnit::Weeks:
      return fixed_span(kWeeks);
    case TimelineSpanUnit::Months:
    case TimelineSpanUnit::Years:
    case TimelineSpanUnit::BusinessDays:
      break;
  }

  const bool reversed = end_ns < start_ns;
  const auto lo_ns = reversed ? end_ns : start_ns;
  const auto hi_ns = reversed ? start_ns : end_ns;
  const auto lo = span_fields_(lo_ns, tz);
  const auto hi = span_fields_(hi_ns, tz);

  std::int64_t magnitude = 0;
  switch (options.unit) {
    case TimelineSpanUnit::Months:
      magnitude = options.mode == TimelineSpanMode::Boundary
                    ? boundary_months_(lo, hi)
                    : complete_months_forward_(lo, hi, options.anchor);
      break;
    case TimelineSpanUnit::Years:
      magnitude = options.mode == TimelineSpanMode::Boundary
                    ? boundary_years_(lo, hi)
                    : complete_years_forward_(lo, hi, options.anchor);
      break;
    case TimelineSpanUnit::BusinessDays:
      magnitude = business_days_forward_(lo_ns, hi_ns, tz, options, holidays);
      break;
    default:
      CODEM_FAIL("Timeline::span: unsupported span unit");
  }

  if (!options.signed_result) {
    return magnitude;
  }
  return reversed ? -magnitude : magnitude;
}

} // namespace

BusinessCalendar BusinessCalendar::monfri() {
  BusinessCalendar cal;
  cal.weekmask = 0b0111110;
  cal.name = "MonFri";
  return cal;
}

BusinessCalendar BusinessCalendar::usa_federal(int year_start, int year_end) {
  check_calendar_year_range_(year_start, year_end, "BusinessCalendar::usa_federal");
  BusinessCalendar cal = monfri();
  cal.name = "US Federal";
  cal.region = "US";
  std::set<std::int64_t> days;
  for (int y = year_start; y <= year_end; ++y) {
    add_observed_fixed_(days, y, 1, 1);   // New Year's Day
    add_holiday_(days, nth_weekday_(y, 1, 1, 3));  // MLK Day
    add_holiday_(days, nth_weekday_(y, 2, 1, 3));  // Washington's Birthday
    add_holiday_(days, last_weekday_(y, 5, 1));    // Memorial Day
    if (y >= 2021) {
      add_observed_fixed_(days, y, 6, 19);          // Juneteenth
    }
    add_observed_fixed_(days, y, 7, 4);             // Independence Day
    add_holiday_(days, nth_weekday_(y, 9, 1, 1));   // Labor Day
    add_holiday_(days, nth_weekday_(y, 10, 1, 2));  // Columbus Day
    add_observed_fixed_(days, y, 11, 11);           // Veterans Day
    add_holiday_(days, nth_weekday_(y, 11, 4, 4));  // Thanksgiving
    add_observed_fixed_(days, y, 12, 25);           // Christmas
  }
  add_unique_holidays_(cal, days);
  return cal;
}

BusinessCalendar BusinessCalendar::canada_federal(int year_start, int year_end) {
  check_calendar_year_range_(year_start, year_end, "BusinessCalendar::canada_federal");
  BusinessCalendar cal = monfri();
  cal.name = "Canada Federal";
  cal.region = "CA";
  std::set<std::int64_t> days;
  for (int y = year_start; y <= year_end; ++y) {
    add_observed_fixed_forward_(days, y, 1, 1);     // New Year's Day
    add_holiday_(days, western_easter_offset_(y, -2)); // Good Friday
    add_holiday_(days, last_weekday_(y, 5, 1));     // Victoria Day, Monday before May 25
    add_observed_fixed_forward_(days, y, 7, 1);     // Canada Day
    add_holiday_(days, nth_weekday_(y, 9, 1, 1));   // Labour Day
    if (y >= 2021) {
      add_observed_fixed_forward_(days, y, 9, 30);  // National Day for Truth and Reconciliation
    }
    add_holiday_(days, nth_weekday_(y, 10, 1, 2));  // Thanksgiving
    add_observed_fixed_forward_(days, y, 11, 11);   // Remembrance Day
    add_christmas_boxing_forward_(days, y);         // Christmas and Boxing Day
  }
  add_unique_holidays_(cal, days);
  return cal;
}

BusinessCalendar BusinessCalendar::uk_bank(int year_start, int year_end) {
  check_calendar_year_range_(year_start, year_end, "BusinessCalendar::uk_bank");
  BusinessCalendar cal = monfri();
  cal.name = "UK Bank Holidays England/Wales";
  cal.region = "GB-EAW";
  cal.timezone = "Europe/London";
  std::set<std::int64_t> days;
  for (int y = year_start; y <= year_end; ++y) {
    add_holiday_(days, observed_fixed_forward_(y, 1, 1));   // New Year's Day substitute forward
    add_holiday_(days, western_easter_offset_(y, -2));      // Good Friday
    add_holiday_(days, western_easter_offset_(y, 1));       // Easter Monday
    add_holiday_(days, nth_weekday_(y, 5, 1, 1));           // Early May bank holiday
    add_holiday_(days, last_weekday_(y, 5, 1));             // Spring bank holiday
    add_holiday_(days, last_weekday_(y, 8, 1));             // Summer bank holiday

    add_christmas_boxing_forward_(days, y);
  }
  add_unique_holidays_(cal, days);
  return cal;
}

BusinessCalendar BusinessCalendar::target2(int year_start, int year_end) {
  check_calendar_year_range_(year_start, year_end, "BusinessCalendar::target2");
  BusinessCalendar cal = monfri();
  cal.name = "TARGET2";
  cal.region = "EU";
  cal.timezone = "UTC";
  std::set<std::int64_t> days;
  for (int y = year_start; y <= year_end; ++y) {
    add_holiday_(days, epoch_day_ymd_(y, 1, 1));
    add_holiday_(days, western_easter_offset_(y, -2)); // Good Friday
    add_holiday_(days, western_easter_offset_(y, 1));  // Easter Monday
    add_holiday_(days, epoch_day_ymd_(y, 5, 1));
    add_holiday_(days, epoch_day_ymd_(y, 12, 25));
    add_holiday_(days, epoch_day_ymd_(y, 12, 26));
  }
  add_unique_holidays_(cal, days);
  return cal;
}

BusinessCalendar BusinessCalendar::nyse(int year_start, int year_end) {
  check_calendar_year_range_(year_start, year_end, "BusinessCalendar::nyse");
  BusinessCalendar cal = monfri();
  cal.name = "NYSE";
  cal.region = "US-NYSE";
  cal.timezone = "America/New_York";
  std::set<std::int64_t> days;
  for (int y = year_start; y <= year_end; ++y) {
    const auto new_year = epoch_day_ymd_(y, 1, 1);
    const int new_year_wd = weekday_sun0_from_ymd_(y, 1, 1);
    if (new_year_wd != 6) { // NYSE does not observe Saturday New Year's Day on prior Friday.
      add_holiday_(days, new_year_wd == 0 ? new_year + 1 : new_year);
    }
    add_holiday_(days, nth_weekday_(y, 1, 1, 3));   // MLK Day
    add_holiday_(days, nth_weekday_(y, 2, 1, 3));   // Washington's Birthday
    add_holiday_(days, western_easter_offset_(y, -2)); // Good Friday
    add_holiday_(days, last_weekday_(y, 5, 1));     // Memorial Day
    if (y >= 2022) {
      add_observed_fixed_(days, y, 6, 19);          // Juneteenth
    }
    add_observed_fixed_(days, y, 7, 4);             // Independence Day
    add_holiday_(days, nth_weekday_(y, 9, 1, 1));   // Labor Day
    add_holiday_(days, nth_weekday_(y, 11, 4, 4));  // Thanksgiving
    add_observed_fixed_(days, y, 12, 25);           // Christmas
  }
  add_unique_holidays_(cal, days);
  return cal;
}

pluss::codem::Series TimelineBase::span(
    const Timeline& other,
    const TimelineSpanOptions& options) const {
  const auto& self = as_timeline(*this);
  check_timeline_core(self.unsafe_ticks());
  check_timeline_core(other.unsafe_ticks());

  validate_span_options_(self, other, options);

  std::set<std::int64_t> holidays(
      options.calendar.holidays.begin(),
      options.calendar.holidays.end());
  const TimeZone tz = TimeZone::load(self.tz());
  const auto shape =
      pluss::codem::SeriesIterator::broadcast_shape(
          self.unsafe_ticks().sizes(),
          other.unsafe_ticks().sizes());
  auto out = pluss::codem::Series::empty(shape, pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::binary(self.unsafe_ticks(), other.unsafe_ticks(), out);
  const auto* ap = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  const auto* bp = reinterpret_cast<const std::int64_t*>(it.b().raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t bo, std::int64_t oo) {
    op[oo] = timeline_span_one_(ap[ao], bp[bo], tz, options, holidays);
  });
  return out;
}

Timeline TimelineBase::concat(const Timeline& other) const {
  const auto& self = as_timeline(*this);
  CODEM_CHECK(std::string_view(self.tz()) == std::string_view(other.tz()),
              "Timeline::concat: timezone mismatch");

  auto shape = self.sizes().vec();
  auto other_shape = other.sizes().vec();
  if (shape.empty()) shape.push_back(1);
  if (other_shape.empty()) other_shape.push_back(1);
  CODEM_CHECK(shape.size() == other_shape.size(),
              "Timeline::concat: operands must have the same rank after scalar promotion");
  CODEM_CHECK(std::equal(shape.begin() + 1, shape.end(),
                         other_shape.begin() + 1, other_shape.end()),
              "Timeline::concat: trailing dimensions must match");
  CODEM_CHECK(other_shape[0] <= std::numeric_limits<std::int64_t>::max() - shape[0],
              "Timeline::concat: axis-0 extent overflows int64");
  shape[0] += other_shape[0];

  auto out = self.ticks();
  const auto rhs_size = static_cast<std::size_t>(other.numel());
  CODEM_CHECK(rhs_size <= std::numeric_limits<std::size_t>::max() - out.size(),
              "Timeline::concat: element count overflows size_t");
  out.reserve(out.size() + rhs_size);
  other.copy_ticks(std::back_inserter(out));
  return ops_internal::timeline_from_series(make_i64_series(out, shape),
                                             std::string(self.tz()));
}

Timeline TimelineBase::concat(const std::vector<Timeline>& others) const {
  const auto& self = as_timeline(*this);
  auto shape = self.sizes().vec();
  if (shape.empty()) shape.push_back(1);
  std::int64_t axis0 = shape[0];
  std::size_t reserve_extra = 0;
  for (const auto& t : others) {
    CODEM_CHECK(std::string_view(self.tz()) == std::string_view(t.tz()),
                "Timeline::concat: timezone mismatch");
    auto other_shape = t.sizes().vec();
    if (other_shape.empty()) other_shape.push_back(1);
    CODEM_CHECK(shape.size() == other_shape.size(),
                "Timeline::concat: operands must have the same rank after scalar promotion");
    CODEM_CHECK(std::equal(shape.begin() + 1, shape.end(),
                           other_shape.begin() + 1, other_shape.end()),
                "Timeline::concat: trailing dimensions must match");
    CODEM_CHECK(other_shape[0] <= std::numeric_limits<std::int64_t>::max() - axis0,
                "Timeline::concat: axis-0 extent overflows int64");
    axis0 += other_shape[0];
    const auto n = static_cast<std::size_t>(t.numel());
    CODEM_CHECK(n <= std::numeric_limits<std::size_t>::max() - reserve_extra,
                "Timeline::concat: element count overflows size_t");
    reserve_extra += n;
  }
  shape[0] = axis0;

  std::vector<std::int64_t> out = self.ticks();
  CODEM_CHECK(reserve_extra <= std::numeric_limits<std::size_t>::max() - out.size(),
              "Timeline::concat: element count overflows size_t");
  out.reserve(out.size() + reserve_extra);
  for (const auto& t : others) {
    t.copy_ticks(std::back_inserter(out));
  }
  return ops_internal::timeline_from_series(make_i64_series(out, shape),
                                             std::string(self.tz()));
}

Timeline TimelineBase::repeat(std::int64_t times) const {
  const auto& self = as_timeline(*this);
  CODEM_CHECK(times >= 0, "Timeline::repeat: times must be non-negative");

  auto shape = self.sizes().vec();
  if (shape.empty()) shape.push_back(1);
  CODEM_CHECK(times == 0 || shape[0] <= std::numeric_limits<std::int64_t>::max() / times,
              "Timeline::repeat: axis-0 extent overflows int64");
  shape[0] *= times;

  const auto flat = self.ticks();
  CODEM_CHECK(times == 0 || flat.size() <=
                  std::numeric_limits<std::size_t>::max() /
                      static_cast<std::size_t>(times),
              "Timeline::repeat: element count overflows size_t");
  std::vector<std::int64_t> out;
  out.reserve(flat.size() * static_cast<std::size_t>(times));
  for (std::int64_t i = 0; i < times; ++i) {
    out.insert(out.end(), flat.begin(), flat.end());
  }
  return ops_internal::timeline_from_series(make_i64_series(out, shape),
                                             std::string(self.tz()));
}

Timeline TimelineBase::empty_like() const {
  const auto& self = as_timeline(*this);
  std::vector<std::int64_t> shape(self.sizes().begin(), self.sizes().end());
  if (shape.empty()) {
    shape.push_back(0);
  } else {
    shape[0] = 0;
  }
  const std::vector<std::int64_t> flat;
  return ops_internal::timeline_from_series(make_i64_series(flat, shape),
                                             std::string(self.tz()));
}

Timeline TimelineBase::fill_like(int year, int month, int day) const {
  const auto& self = as_timeline(*this);
  const auto ymd = std::chrono::year{year} /
                   std::chrono::month{static_cast<unsigned>(month)} /
                   std::chrono::day{static_cast<unsigned>(day)};
  CODEM_CHECK(ymd.ok(), "Timeline::fill_like: invalid calendar date");
  const auto local = std::chrono::sys_days{ymd};
  const auto local_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(local.time_since_epoch()).count();
  const auto fill_tick =
      TimeZone::load(self.tz()).local_to_utc(local_ns, 0, "shift_forward");

  std::vector<std::int64_t> shape(self.sizes().begin(), self.sizes().end());
  const std::size_t n = static_cast<std::size_t>(self.numel());
  std::vector<std::int64_t> flat(n, fill_tick);
  return ops_internal::timeline_from_series(make_i64_series(flat, shape),
                                             std::string(self.tz()));
}

Timeline TimelineBase::fill_like(const std::string& date_str) const {
  const auto value = trim(date_str);
  const bool date_only = value.size() == 10 &&
      (value[4] == '-' || value[4] == '/') &&
      (value[7] == '-' || value[7] == '/') &&
      std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isdigit(c) || c == '-' || c == '/';
      });
  CODEM_CHECK(date_only,
              "Timeline::fill_like: expected a date-only token (YYYY-MM-DD)");
  const auto parsed = parse_datetime(value);
  CODEM_CHECK(parsed.ok, "Timeline::fill_like: invalid date token");
  return fill_like(parsed.y, parsed.mo, parsed.d);
}

pluss::codem::Series TimelineBase::operator==(const Timeline& other) const {
  return detail::impl_cmp(as_timeline(*this), other, "eq");
}

pluss::codem::Series TimelineBase::operator!=(const Timeline& other) const {
  return detail::impl_cmp(as_timeline(*this), other, "ne");
}

pluss::codem::Series TimelineBase::operator<(const Timeline& other) const {
  return detail::impl_cmp(as_timeline(*this), other, "lt");
}

pluss::codem::Series TimelineBase::operator<=(const Timeline& other) const {
  return detail::impl_cmp(as_timeline(*this), other, "le");
}

pluss::codem::Series TimelineBase::operator>(const Timeline& other) const {
  return detail::impl_cmp(as_timeline(*this), other, "gt");
}

pluss::codem::Series TimelineBase::operator>=(const Timeline& other) const {
  return detail::impl_cmp(as_timeline(*this), other, "ge");
}

Timeline TimelineBase::shift(int periods, std::string_view freq) const {
  const auto f = lower_ascii(freq);
  if (f == "year") {
    return add_years(periods);
  }
  if (f == "month") {
    return add_months(periods);
  }
  if (f == "day") {
    return add_days(periods);
  }
  if (f == "hour") {
    return add_hours(periods);
  }
  if (f == "minute") {
    return add_minutes(periods);
  }
  if (f == "second") {
    return add_seconds(periods);
  }
  CODEM_FAIL("Timeline::shift: unsupported frequency");
}

Timeline TimelineBase::period_start(std::string_view unit) const {
  const auto& self = as_timeline(*this);
  const auto normalized = lower_ascii(unit);
  if (normalized == "day") {
    return detail::impl_dt_round(self, "floor", "day");
  }
  if (normalized == "hour") {
    return detail::impl_dt_round(self, "floor", "hour");
  }
  if (normalized == "minute") {
    return detail::impl_dt_round(self, "floor", "min");
  }
  if (normalized == "second") {
    return detail::impl_dt_round(self, "floor", "second");
  }
  CODEM_CHECK(normalized == "year" || normalized == "month",
              "Timeline::period_start: unsupported unit; expected year, month, day, hour, minute, or second");

  const auto timezone = TimeZone::load(self.tz());
  auto out = pluss::codem::Series::empty(self.sizes(), pluss::codem::ScalarType::Int64);
  auto iterator = pluss::codem::SeriesIterator::unary(self.unsafe_ticks(), out);
  const auto* input = reinterpret_cast<const std::int64_t*>(iterator.a().raw_data());
  auto* output = reinterpret_cast<std::int64_t*>(out.raw_data());

  iterator.for_each_index([&](std::int64_t, std::int64_t input_offset,
                              std::int64_t, std::int64_t output_offset) {
    const auto tick = input[input_offset];
    if (tick == Timeline::kNaT) {
      output[output_offset] = Timeline::kNaT;
      return;
    }

    int year_value, month_value, day_value, hour_value, minute_value;
    int second_value, nanosecond_value;
    epoch_nanos_to_local_fields(tick, timezone, year_value, month_value, day_value,
                                hour_value, minute_value, second_value,
                                nanosecond_value);
    if (normalized == "year") {
      month_value = 1;
    }
    const auto first_day = std::chrono::year{year_value} /
                           std::chrono::month{static_cast<unsigned>(month_value)} /
                           std::chrono::day{1};
    const auto local_midnight = std::chrono::sys_days(first_day);
    output[output_offset] = timezone.local_to_utc(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            local_midnight.time_since_epoch()).count(),
        0, "raise");
  });

  return pluss::codeh::ops_internal::timeline_from_series(
      std::move(out), std::string(self.tz()));
}

TimelineIndexAccessor TimelineBase::index(bool require_sorted) const {
  return ::pluss::codeh::_compat::invoke<TimelineIndexAccessor>("make_index_accessor",
                                                     as_timeline(*this),
                                                     {require_sorted});
}

Timeline TimelineBase::from_ymd(const pluss::codem::Series& year,
                                const pluss::codem::Series& month,
                                const pluss::codem::Series& day,
                                std::string_view tz_name) {
  const auto y = load_component_i64(year, "from_ymd(year)");
  const auto mo = load_component_i64(month, "from_ymd(month)");
  const auto d = load_component_i64(day, "from_ymd(day)");
  const ComponentInput hh{std::vector<std::int64_t>{0}, {}, true};
  const ComponentInput mm{std::vector<std::int64_t>{0}, {}, true};
  const ComponentInput ss{std::vector<std::int64_t>{0}, {}, true};
  const std::vector<ComponentInput> all{y, mo, d, hh, mm, ss};
  const auto shape = broadcast_or_same_shape(all, "Timeline::from_ymd");
  std::int64_t n = 1;
  for (const auto s : shape) n *= s;
  if (shape.empty()) n = 1;

  const auto tz = TimeZone::load(tz_name);
  std::vector<std::int64_t> out(static_cast<std::size_t>(n), Timeline::kNaT);
  for (std::int64_t i = 0; i < n; ++i) {
    const auto yy = pick_component(y, i);
    const auto mmv = pick_component(mo, i);
    const auto dd = pick_component(d, i);
    if (yy == Timeline::kNaT || mmv == Timeline::kNaT || dd == Timeline::kNaT) {
      continue;
    }
    const auto ymd = std::chrono::year(static_cast<int>(yy))
                   / std::chrono::month(static_cast<unsigned>(mmv))
                   / std::chrono::day(static_cast<unsigned>(dd));
    CODEM_CHECK(ymd.ok(),
                "Timeline::from_ymd: invalid calendar date at logical position " +
                    std::to_string(i));
    const auto local = sys_days(ymd);
    const auto local_ns =
        duration_cast<nanoseconds>(local.time_since_epoch()).count();
    out[static_cast<std::size_t>(i)] = tz.local_to_utc(local_ns, 0, "shift_forward");
  }

  Timeline t = ops_internal::timeline_from_epoch_nanos(out, shape);
  return detail::impl_tz_convert(t, tz_name);
}

Timeline TimelineBase::from_ymdhms(const pluss::codem::Series& year,
                                   const pluss::codem::Series& month,
                                   const pluss::codem::Series& day,
                                   const pluss::codem::Series& hour,
                                   const pluss::codem::Series& minute,
                                   const pluss::codem::Series& second,
                                   std::string_view tz_name) {
  const auto y = load_component_i64(year, "from_ymdhms(year)");
  const auto mo = load_component_i64(month, "from_ymdhms(month)");
  const auto d = load_component_i64(day, "from_ymdhms(day)");
  const auto hh = load_component_i64(hour, "from_ymdhms(hour)");
  const auto mm = load_component_i64(minute, "from_ymdhms(minute)");
  const auto ss = load_component_i64(second, "from_ymdhms(second)");

  const std::vector<ComponentInput> all{y, mo, d, hh, mm, ss};
  const auto shape = broadcast_or_same_shape(all, "Timeline::from_ymdhms");
  std::int64_t n = 1;
  for (const auto s : shape) n *= s;
  if (shape.empty()) n = 1;

  const auto tz = TimeZone::load(tz_name);
  std::vector<std::int64_t> out(static_cast<std::size_t>(n), Timeline::kNaT);
  for (std::int64_t i = 0; i < n; ++i) {
    const auto yy = pick_component(y, i);
    const auto mmv = pick_component(mo, i);
    const auto dd = pick_component(d, i);
    const auto hhv = pick_component(hh, i);
    const auto mmn = pick_component(mm, i);
    const auto ssv = pick_component(ss, i);
    if (yy == Timeline::kNaT || mmv == Timeline::kNaT || dd == Timeline::kNaT ||
        hhv == Timeline::kNaT || mmn == Timeline::kNaT || ssv == Timeline::kNaT) {
      continue;
    }
    CODEM_CHECK(hhv >= 0 && hhv <= 23 && mmn >= 0 && mmn <= 59 &&
                    ssv >= 0 && ssv <= 59,
                "Timeline::from_ymdhms: invalid clock field at logical position " +
                    std::to_string(i));
    const auto ymd = std::chrono::year(static_cast<int>(yy))
                   / std::chrono::month(static_cast<unsigned>(mmv))
                   / std::chrono::day(static_cast<unsigned>(dd));
    CODEM_CHECK(ymd.ok(),
                "Timeline::from_ymdhms: invalid calendar date at logical position " +
                    std::to_string(i));
    const auto local = sys_days(ymd)
                     + hours(static_cast<int>(hhv))
                     + minutes(static_cast<int>(mmn))
                     + seconds(static_cast<int>(ssv));
    const auto local_ns =
        duration_cast<nanoseconds>(local.time_since_epoch()).count();
    out[static_cast<std::size_t>(i)] = tz.local_to_utc(local_ns, 0, "shift_forward");
  }

  Timeline t = ops_internal::timeline_from_epoch_nanos(out, shape);
  return detail::impl_tz_convert(t, tz_name);
}

pluss::codem::Series TimelineBase::epoch_nanos() const {
  const auto& self = as_timeline(*this);
  auto flat = copy_epoch_scaled(self, 1);
  std::vector<std::int64_t> shape(self.sizes().begin(), self.sizes().end());
  return make_i64_series(flat, shape);
}

pluss::codem::Series TimelineBase::epoch_micros() const {
  const auto& self = as_timeline(*this);
  auto flat = copy_epoch_scaled(self, 1'000LL);
  std::vector<std::int64_t> shape(self.sizes().begin(), self.sizes().end());
  return make_i64_series(flat, shape);
}

pluss::codem::Series TimelineBase::epoch_millis() const {
  const auto& self = as_timeline(*this);
  auto flat = copy_epoch_scaled(self, 1'000'000LL);
  std::vector<std::int64_t> shape(self.sizes().begin(), self.sizes().end());
  return make_i64_series(flat, shape);
}

pluss::codem::Series TimelineBase::epoch_seconds() const {
  const auto& self = as_timeline(*this);
  auto flat = copy_epoch_scaled(self, 1'000'000'000LL);
  std::vector<std::int64_t> shape(self.sizes().begin(), self.sizes().end());
  return make_i64_series(flat, shape);
}

pluss::codem::Series TimelineBase::epoch_days() const {
  const auto& self = as_timeline(*this);
  auto flat = copy_epoch_scaled(self, 86'400'000'000'000LL);
  std::vector<std::int64_t> shape(self.sizes().begin(), self.sizes().end());
  return make_i64_series(flat, shape);
}

Timeline TimelineBase::from_epoch_seconds(const pluss::codem::Series& epoch_seconds,
                                          std::string_view tz_name) {
  const auto c = load_component_i64(epoch_seconds, "Timeline::from_epoch_seconds");
  std::vector<std::int64_t> flat(c.flat.size(), Timeline::kNaT);
  for (std::size_t i = 0; i < flat.size(); ++i) {
    if (c.flat[i] != Timeline::kNaT) {
      flat[i] = checked_epoch_scale(c.flat[i], 1'000'000'000LL,
                                    "Timeline::from_epoch_seconds");
    }
  }
  Timeline t = ops_internal::timeline_from_epoch_nanos(flat, c.shape);
  return detail::impl_tz_convert(t, tz_name);
}

Timeline TimelineBase::from_epoch_millis(const pluss::codem::Series& epoch_millis,
                                         std::string_view tz_name) {
  const auto c = load_component_i64(epoch_millis, "Timeline::from_epoch_millis");
  std::vector<std::int64_t> flat(c.flat.size(), Timeline::kNaT);
  for (std::size_t i = 0; i < flat.size(); ++i) {
    if (c.flat[i] != Timeline::kNaT) {
      flat[i] = checked_epoch_scale(c.flat[i], 1'000'000LL,
                                    "Timeline::from_epoch_millis");
    }
  }
  Timeline t = ops_internal::timeline_from_epoch_nanos(flat, c.shape);
  return detail::impl_tz_convert(t, tz_name);
}

Timeline TimelineBase::from_epoch_micros(const pluss::codem::Series& epoch_micros,
                                         std::string_view tz_name) {
  const auto c = load_component_i64(epoch_micros, "Timeline::from_epoch_micros");
  std::vector<std::int64_t> flat(c.flat.size(), Timeline::kNaT);
  for (std::size_t i = 0; i < flat.size(); ++i) {
    if (c.flat[i] != Timeline::kNaT) {
      flat[i] = checked_epoch_scale(c.flat[i], 1'000LL,
                                    "Timeline::from_epoch_micros");
    }
  }
  Timeline t = ops_internal::timeline_from_epoch_nanos(flat, c.shape);
  return detail::impl_tz_convert(t, tz_name);
}

Timeline TimelineBase::now() {
  const auto ns = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
  return scalar_from_utc_ns(ns, "UTC");
}

Timeline TimelineBase::today() { return now().period_start("day"); }
Timeline TimelineBase::yesterday() { return today().add_days(-1); }
Timeline TimelineBase::tomorrow() { return today().add_days(1); }

std::vector<Timeline> TimelineBase::date_range(const Timeline& start,
                                               const Timeline& end,
                                               std::string_view freq,
                                               int step) {
  CODEM_CHECK(step != 0, "Timeline::date_range: step must be non-zero");
  CODEM_CHECK(start.dim() == 0 && end.dim() == 0,
              "Timeline::date_range: scalar start/end required");
  std::vector<Timeline> out;
  Timeline cur = start.period_start(freq);
  const Timeline endp = end.period_start(freq);
  if (step > 0) {
    while (scalar_bool_series(cur.le(endp), "Timeline::date_range")) {
      out.push_back(cur);
      cur = cur.shift(step, freq);
    }
  } else {
    while (scalar_bool_series(cur.ge(endp), "Timeline::date_range")) {
      out.push_back(cur);
      cur = cur.shift(step, freq);
    }
  }
  return out;
}

} // namespace pluss::codeh
