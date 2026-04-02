
#include "CodeH/core/Timeline.h"
#include "CodeH/core/TimeIndex.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstring>
#include <cmath>
#include <iomanip>
#include <limits>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "CodeH/core/Offsets.h"
#include "CodeH/core/TimeZone.h"
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
    p.hh = std::stoi(time_part.substr(0, 2));
    p.mm = std::stoi(time_part.substr(3, 2));
    p.ss = 0;
    p.ns = 0;

    if (time_part.size() > 5) {
      if (time_part[5] != ':') return p;
      if (time_part.size() < 8) return p;
      p.ss = std::stoi(time_part.substr(6, 2));
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
  if (p.ss < 0 || p.ss > 60) return p;
  if (p.ns < 0 || p.ns > 999'999'999) return p;

  p.ok = true;
  return p;
}

static std::int64_t to_epoch_nanos_utc(const ParsedDT& p) {
  const auto day = std::chrono::sys_days(std::chrono::year(p.y) / std::chrono::month((unsigned)p.mo) / std::chrono::day((unsigned)p.d));
  const auto tp = day + std::chrono::hours(p.hh) + std::chrono::minutes(p.mm) + std::chrono::seconds(p.ss) + std::chrono::nanoseconds(p.ns);
  auto utc = tp;
  if (p.has_tz) utc -= std::chrono::minutes(p.tz_offset_min);
  return utc.time_since_epoch().count();
}

static inline std::int64_t unit_nanos(std::string_view unit) {
  if (unit == "S" || unit == "s" || unit == "sec" || unit == "second") return 1'000'000'000LL;
  if (unit == "min" || unit == "m" || unit == "minute") return 60LL * 1'000'000'000LL;
  if (unit == "H" || unit == "h" || unit == "hour") return 3600LL * 1'000'000'000LL;
  if (unit == "D" || unit == "d" || unit == "day") return 86400LL * 1'000'000'000LL;
  CODEM_FAIL("Timeline: unsupported unit (expected S/min/H/D)");
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

static std::string strftime_one(std::int64_t ns_utc, const TimeZone& tz, std::string_view fmt) {
  if (ns_utc == Timeline::kNaT) return "NaT";
  int y, mo, d, hh, mm, ss, nsec, off_min;
  epoch_nanos_to_local_fields(ns_utc, tz, y, mo, d, hh, mm, ss, nsec, &off_min);
  std::ostringstream out;
  for (size_t i = 0; i < fmt.size(); ++i) {
    char c = fmt[i];
    if (c != '%') { out << c; continue; }
    if (i + 1 >= fmt.size()) { out << '%'; break; }
    char t = fmt[++i];
    switch (t) {
      case '%': out << '%'; break;
      case 'Y': out << std::setw(4) << std::setfill('0') << y; break;
      case 'm': out << std::setw(2) << std::setfill('0') << mo; break;
      case 'd': out << std::setw(2) << std::setfill('0') << d; break;
      case 'H': out << std::setw(2) << std::setfill('0') << hh; break;
      case 'M': out << std::setw(2) << std::setfill('0') << mm; break;
      case 'S': out << std::setw(2) << std::setfill('0') << ss; break;
      case 'f': out << std::setw(9) << std::setfill('0') << nsec; break;
      case 'z': out << zoffset_str(off_min); break;
      case 'a': { // short weekday
        static const char* names[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
        const auto dp = sys_days(year(y)/month((unsigned)mo)/day((unsigned)d));
        int wd = (int)weekday(dp).c_encoding(); // Sun=0
        int mon0 = (wd + 6) % 7;
        out << names[mon0];
      } break;
      case 'A': {
        static const char* names[] = {"Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday"};
        const auto dp = sys_days(year(y)/month((unsigned)mo)/day((unsigned)d));
        int wd = (int)weekday(dp).c_encoding();
        int mon0 = (wd + 6) % 7;
        out << names[mon0];
      } break;
      case 'b': {
        static const char* names[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
        out << names[mo-1];
      } break;
      case 'B': {
        static const char* names[] = {"January","February","March","April","May","June","July","August","September","October","November","December"};
        out << names[mo-1];
      } break;
      default:
        // Unknown token: emit literally.
        out << '%' << t;
        break;
    }
  }
  return out.str();
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
    // Some Series backends keep empty tensors as undefined storage if built via
    // empty(...). Route empty shape construction through make_from_doubles to
    // preserve a defined, shape-carrying handle.
    auto s = pluss::codem::Series::make_from_doubles({}, shape, pluss::codem::ScalarType::Int64);
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

Timeline impl_add_duration(const Timeline& a, const Duration& d) {
  check_timeline_core(a.ticks_);
  CODEM_CHECK(d.defined(), "Timeline + Duration: duration undefined");
  CODEM_CHECK(d.nanos_.scalar_type() == pluss::codem::ScalarType::Int64, "Timeline + Duration: nanos must be int64");
  const auto shape = pluss::codem::SeriesIterator::broadcast_shape(a.ticks_.sizes(), d.nanos_.sizes());
  auto out = pluss::codem::Series::empty(shape, pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::binary(a.ticks_, d.nanos_, out);
  const auto* ap = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  const auto* bp = reinterpret_cast<const std::int64_t*>(it.b().raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t bo, std::int64_t oo) {
    const auto av = ap[ao];
    const auto bv = bp[bo];
    op[oo] = (av == Timeline::kNaT || bv == Duration::kNaT) ? Timeline::kNaT : (av + bv);
  });
  return Timeline(std::move(out), std::string(a.tz()));
}

Timeline impl_sub_duration(const Timeline& a, const Duration& d) {
  check_timeline_core(a.ticks_);
  CODEM_CHECK(d.defined(), "Timeline - Duration: duration undefined");
  CODEM_CHECK(d.nanos_.scalar_type() == pluss::codem::ScalarType::Int64, "Timeline - Duration: nanos must be int64");
  const auto shape = pluss::codem::SeriesIterator::broadcast_shape(a.ticks_.sizes(), d.nanos_.sizes());
  auto out = pluss::codem::Series::empty(shape, pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::binary(a.ticks_, d.nanos_, out);
  const auto* ap = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  const auto* bp = reinterpret_cast<const std::int64_t*>(it.b().raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t bo, std::int64_t oo) {
    const auto av = ap[ao];
    const auto bv = bp[bo];
    op[oo] = (av == Timeline::kNaT || bv == Duration::kNaT) ? Timeline::kNaT : (av - bv);
  });
  return Timeline(std::move(out), std::string(a.tz()));
}

Duration impl_diff(const Timeline& a, const Timeline& b) {
  check_timeline_core(a.ticks_);
  check_timeline_core(b.ticks_);
  const auto shape = pluss::codem::SeriesIterator::broadcast_shape(a.ticks_.sizes(), b.ticks_.sizes());
  auto out = pluss::codem::Series::empty(shape, pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::binary(a.ticks_, b.ticks_, out);
  const auto* ap = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  const auto* bp = reinterpret_cast<const std::int64_t*>(it.b().raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t bo, std::int64_t oo) {
    const auto av = ap[ao];
    const auto bv = bp[bo];
    op[oo] = (av == Timeline::kNaT || bv == Timeline::kNaT) ? Duration::kNaT : (av - bv);
  });
  return Duration(out);
}

pluss::codem::Series impl_cmp(const Timeline& a, const Timeline& b, std::string_view op) {
  check_timeline_core(a.ticks_);
  check_timeline_core(b.ticks_);
  const auto shape = pluss::codem::SeriesIterator::broadcast_shape(a.ticks_.sizes(), b.ticks_.sizes());
  auto out = pluss::codem::Series::empty(shape, pluss::codem::ScalarType::Bool);
  auto it = pluss::codem::SeriesIterator::binary(a.ticks_, b.ticks_, out);
  const auto* ap = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  const auto* bp = reinterpret_cast<const std::int64_t*>(it.b().raw_data());
  auto* optr = reinterpret_cast<bool*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t bo, std::int64_t oo) {
    const auto av = ap[ao];
    const auto bv = bp[bo];
    if (av == Timeline::kNaT || bv == Timeline::kNaT) { optr[oo] = false; return; }
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
    // Round in local wall time, then map back using the original UTC offset at v.
    const int off = tz.offset_minutes_at_utc(v);
    const std::int64_t local = v + (std::int64_t)off * 60LL * 1'000'000'000LL;

    std::int64_t qv = 0;
    if (mode == "floor") {
      qv = (local / q) * q;
    } else if (mode == "ceil") {
      qv = ((local + q - 1) / q) * q;
    } else if (mode == "round") {
      qv = ((local + q / 2) / q) * q;
    } else {
      CODEM_FAIL("Timeline round: unknown mode");
    }
    op[oo] = qv - (std::int64_t)off * 60LL * 1'000'000'000LL;
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
  check_timeline_core(a.ticks_);
  const auto tz = TimeZone::load(a.tz());
  const auto n = a.numel();
  std::vector<std::string> storage;
  storage.reserve((size_t)n);
  std::vector<std::string_view> views;
  views.reserve((size_t)n);

  const auto* x = reinterpret_cast<const std::int64_t*>(a.ticks_.raw_data());
  for (std::int64_t i = 0; i < n; ++i) {
    storage.push_back(strftime_one(x[i], tz, fmt));
  }
  for (auto& s : storage) views.push_back(s);

  return pluss::make_note(views, a.ticks_.sizes());
}

Timeline impl_date_range(const Timeline& start, const Timeline& end, std::string_view freq, std::int64_t step) {
  check_timeline_core(start.ticks_);
  check_timeline_core(end.ticks_);
  CODEM_CHECK(start.dim() == 0 && end.dim() == 0, "date_range requires scalar start/end");
  CODEM_CHECK(step != 0, "date_range step must be non-zero");
  const auto q = unit_nanos(freq);
  const auto s = reinterpret_cast<const std::int64_t*>(start.ticks_.raw_data())[0];
  const auto e = reinterpret_cast<const std::int64_t*>(end.ticks_.raw_data())[0];
  CODEM_CHECK(s != Timeline::kNaT && e != Timeline::kNaT, "date_range does not support NaT endpoints");

  std::vector<std::int64_t> out;
  if (step > 0) {
    for (std::int64_t v = s; v <= e; v += q * step) out.push_back(v);
  } else {
    for (std::int64_t v = s; v >= e; v += q * step) out.push_back(v);
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
    int y, mo, d, hh, mm, ss, nsec, off;
    epoch_nanos_to_local_fields(v, tz, y, mo, d, hh, mm, ss, nsec, &off);
    std::int64_t tod = (std::int64_t)hh * 3600LL*1'000'000'000LL + (std::int64_t)mm * 60LL*1'000'000'000LL + (std::int64_t)ss * 1'000'000'000LL + nsec;
    // return as UTC instant at base day + tod, keeping tz metadata.
    op[oo] = base + tod - (std::int64_t)off * 60LL * 1'000'000'000LL;
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

Timeline impl_min(const Timeline& a) {
  check_timeline_core(a.ticks_);
  const auto n = a.numel();
  const auto* x = reinterpret_cast<const std::int64_t*>(a.ticks_.raw_data());
  std::int64_t best = Timeline::kNaT;
  for (std::int64_t i = 0; i < n; ++i) {
    const auto v = x[i];
    if (v == Timeline::kNaT) continue;
    if (best == Timeline::kNaT || v < best) best = v;
  }
  return impl_tz_convert(
      ops_internal::timeline_from_epoch_nanos(
          std::span<const std::int64_t>(&best, 1), {}),
      a.tz());
}

Timeline impl_max(const Timeline& a) {
  check_timeline_core(a.ticks_);
  const auto n = a.numel();
  const auto* x = reinterpret_cast<const std::int64_t*>(a.ticks_.raw_data());
  std::int64_t best = Timeline::kNaT;
  for (std::int64_t i = 0; i < n; ++i) {
    const auto v = x[i];
    if (v == Timeline::kNaT) continue;
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
  std::vector<std::int64_t> x((size_t)a.numel());
  std::memcpy(x.data(), a.ticks_.raw_data(), (size_t)a.numel()*sizeof(std::int64_t));
  std::stable_sort(x.begin(), x.end());
  x.erase(std::unique(x.begin(), x.end()), x.end());
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(x.size())};
  return impl_tz_convert(ops_internal::timeline_from_epoch_nanos(x, shape),
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
  const auto* x = reinterpret_cast<const std::int64_t*>(a.ticks_.raw_data());
  auto out = pluss::codem::Series::empty(values.ticks_.sizes(), pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::unary(values.ticks_, out);
  const auto* vp = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    const auto k = vp[ao];
    if (k == Timeline::kNaT) { op[oo] = -1; return; }
    if (left) op[oo] = (std::int64_t)(std::lower_bound(x, x+n, k) - x);
    else op[oo] = (std::int64_t)(std::upper_bound(x, x+n, k) - x);
  });
  return out;
}

Timeline impl_add_nanos(const Timeline& a, std::int64_t nanos) {
  check_timeline_core(a.ticks_);
  auto out = pluss::codem::Series::empty(a.ticks_.sizes(), pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::unary(a.ticks_, out);
  const auto* ap = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    const auto v = ap[ao];
    op[oo] = (v == Timeline::kNaT) ? Timeline::kNaT : (v + nanos);
  });
  return Timeline(std::move(out), std::string(a.tz()));
}

std::vector<std::int64_t> impl_to_epoch_nanos_flat(const Timeline& a) {
  check_timeline_core(a.ticks_);
  const auto n = a.numel();
  std::vector<std::int64_t> out((size_t)n);
  std::memcpy(out.data(), a.ticks_.raw_data(), (size_t)n * sizeof(std::int64_t));
  return out;
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

static bool all_true_bool_series(const pluss::codem::Series& s, std::string_view where) {
  CODEM_CHECK(s.defined(), std::string(where) + ": undefined Series");
  CODEM_CHECK(s.scalar_type() == pluss::codem::ScalarType::Bool,
              std::string(where) + ": expected bool Series");
  if (s.numel() == 0) return true;
  const auto c = s.contiguous();
  const auto* p = reinterpret_cast<const bool*>(c.raw_data());
  for (std::int64_t i = 0; i < c.numel(); ++i) {
    if (!p[i]) return false;
  }
  return true;
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
  auto ns = detail::impl_to_epoch_nanos_flat(t);
  if (denom == 1) {
    return ns;
  }
  for (auto& v : ns) {
    if (v != Timeline::kNaT) {
      v /= denom;
    }
  }
  return ns;
}

static pluss::codem::Series make_i64_series(std::span<const std::int64_t> flat,
                                            std::span<const std::int64_t> shape) {
  auto out = pluss::codem::Series::empty(shape, pluss::codem::ScalarType::Int64);
  if (!flat.empty()) {
    std::memcpy(out.raw_data(), flat.data(), flat.size() * sizeof(std::int64_t));
  }
  return out;
}

static pluss::codem::Series full_i64_like(const Timeline& t, std::int64_t value) {
  return pluss::codem::Series::full(t.sizes(),
                                    static_cast<double>(value),
                                    pluss::codem::ScalarType::Int64);
}

static Timeline set_scalar_local_fields(const Timeline& self,
                                        int y, int mo, int d,
                                        int hh, int mm, int ss) {
  CODEM_CHECK(self.dim() == 0, "Timeline set_*: scalar-only");
  const auto ns_utc = scalar_tick_utc(self);
  if (ns_utc == Timeline::kNaT) {
    return self;
  }

  mo = std::clamp(mo, 1, 12);
  hh = std::clamp(hh, 0, 23);
  mm = std::clamp(mm, 0, 59);
  ss = std::clamp(ss, 0, 59);

  const unsigned max_day = days_in_month_ym(y, static_cast<unsigned>(mo));
  d = std::clamp(d, 1, static_cast<int>(max_day));

  const auto tz = TimeZone::load(self.tz());
  const auto local_dp = sys_days(year{y} / month{static_cast<unsigned>(mo)} /
                                 day{static_cast<unsigned>(d)});
  const auto local_tp = local_dp + hours(hh) + minutes(mm) + seconds(ss);
  const std::int64_t local_ns =
      duration_cast<nanoseconds>(local_tp.time_since_epoch()).count();
  const std::int64_t utc = tz.local_to_utc(local_ns, 0, "shift_forward");
  return scalar_from_utc_ns(utc, self.tz());
}

} // namespace

bool TimelineBase::has_date() const noexcept { return defined(); }
bool TimelineBase::has_time() const noexcept { return defined(); }

int TimelineBase::year_scalar() const { return scalar_field(as_timeline(*this), "year"); }
int TimelineBase::month_scalar() const { return scalar_field(as_timeline(*this), "month"); }
int TimelineBase::day_scalar() const { return scalar_field(as_timeline(*this), "day"); }
int TimelineBase::hour_scalar() const { return scalar_field(as_timeline(*this), "hour"); }
int TimelineBase::minute_scalar() const { return scalar_field(as_timeline(*this), "minute"); }
int TimelineBase::second_scalar() const { return scalar_field(as_timeline(*this), "second"); }
int TimelineBase::day_of_week_scalar() const {
  const int mon0 = scalar_field(as_timeline(*this), "weekday");
  return (mon0 + 1) % 7; // Sunday=0 compatibility
}
int TimelineBase::day_of_year_scalar() const { return scalar_field(as_timeline(*this), "dayofyear"); }
int TimelineBase::quarter_scalar() const { return scalar_field(as_timeline(*this), "quarter"); }
int TimelineBase::week_of_year_scalar() const { return scalar_field(as_timeline(*this), "week"); }

bool TimelineBase::is_leap_year_scalar() const { return scalar_pred(as_timeline(*this), "is_leap_year"); }
bool TimelineBase::is_month_start_scalar() const { return scalar_pred(as_timeline(*this), "is_month_start"); }
bool TimelineBase::is_month_end_scalar() const { return scalar_pred(as_timeline(*this), "is_month_end"); }
bool TimelineBase::is_quarter_start_scalar() const { return scalar_pred(as_timeline(*this), "is_quarter_start"); }
bool TimelineBase::is_quarter_end_scalar() const { return scalar_pred(as_timeline(*this), "is_quarter_end"); }
bool TimelineBase::is_weekend_scalar() const { return scalar_pred(as_timeline(*this), "is_weekend"); }

std::string TimelineBase::weekday_name(bool abbreviated) const {
  return format(abbreviated ? "%a" : "%A");
}

std::string TimelineBase::month_name(bool abbreviated) const {
  return format(abbreviated ? "%b" : "%B");
}

Timeline TimelineBase::set_year(int year) const {
  const auto& self = as_timeline(*this);
  if (self.dim() == 0) {
    return set_scalar_local_fields(self, year, month_scalar(), day_scalar(),
                                   hour_scalar(), minute_scalar(), second_scalar());
  }
  return Timeline::from_ymdhms(
      full_i64_like(self, static_cast<std::int64_t>(year)),
      detail::impl_dt_field(self, "month"),
      detail::impl_dt_field(self, "day"),
      detail::impl_dt_field(self, "hour"),
      detail::impl_dt_field(self, "minute"),
      detail::impl_dt_field(self, "second"),
      self.tz());
}

Timeline TimelineBase::set_month(int month) const {
  const auto& self = as_timeline(*this);
  if (self.dim() == 0) {
    return set_scalar_local_fields(self, year_scalar(), month, day_scalar(),
                                   hour_scalar(), minute_scalar(), second_scalar());
  }
  return Timeline::from_ymdhms(
      detail::impl_dt_field(self, "year"),
      full_i64_like(self, static_cast<std::int64_t>(month)),
      detail::impl_dt_field(self, "day"),
      detail::impl_dt_field(self, "hour"),
      detail::impl_dt_field(self, "minute"),
      detail::impl_dt_field(self, "second"),
      self.tz());
}

Timeline TimelineBase::set_day(int day) const {
  const auto& self = as_timeline(*this);
  if (self.dim() == 0) {
    return set_scalar_local_fields(self, year_scalar(), month_scalar(), day,
                                   hour_scalar(), minute_scalar(), second_scalar());
  }
  return Timeline::from_ymdhms(
      detail::impl_dt_field(self, "year"),
      detail::impl_dt_field(self, "month"),
      full_i64_like(self, static_cast<std::int64_t>(day)),
      detail::impl_dt_field(self, "hour"),
      detail::impl_dt_field(self, "minute"),
      detail::impl_dt_field(self, "second"),
      self.tz());
}

Timeline TimelineBase::set_hour(int hour) const {
  const auto& self = as_timeline(*this);
  if (self.dim() == 0) {
    return set_scalar_local_fields(self, year_scalar(), month_scalar(), day_scalar(),
                                   hour, minute_scalar(), second_scalar());
  }
  return Timeline::from_ymdhms(
      detail::impl_dt_field(self, "year"),
      detail::impl_dt_field(self, "month"),
      detail::impl_dt_field(self, "day"),
      full_i64_like(self, static_cast<std::int64_t>(hour)),
      detail::impl_dt_field(self, "minute"),
      detail::impl_dt_field(self, "second"),
      self.tz());
}

Timeline TimelineBase::set_minute(int minute) const {
  const auto& self = as_timeline(*this);
  if (self.dim() == 0) {
    return set_scalar_local_fields(self, year_scalar(), month_scalar(), day_scalar(),
                                   hour_scalar(), minute, second_scalar());
  }
  return Timeline::from_ymdhms(
      detail::impl_dt_field(self, "year"),
      detail::impl_dt_field(self, "month"),
      detail::impl_dt_field(self, "day"),
      detail::impl_dt_field(self, "hour"),
      full_i64_like(self, static_cast<std::int64_t>(minute)),
      detail::impl_dt_field(self, "second"),
      self.tz());
}

Timeline TimelineBase::set_second(int second) const {
  const auto& self = as_timeline(*this);
  if (self.dim() == 0) {
    return set_scalar_local_fields(self, year_scalar(), month_scalar(), day_scalar(),
                                   hour_scalar(), minute_scalar(), second);
  }
  return Timeline::from_ymdhms(
      detail::impl_dt_field(self, "year"),
      detail::impl_dt_field(self, "month"),
      detail::impl_dt_field(self, "day"),
      detail::impl_dt_field(self, "hour"),
      detail::impl_dt_field(self, "minute"),
      full_i64_like(self, static_cast<std::int64_t>(second)),
      self.tz());
}

std::string TimelineBase::format(const std::string& format_str) const {
  return scalar_note(detail::impl_strftime(as_timeline(*this), format_str), "Timeline::format");
}

std::string TimelineBase::toISOString() const { return detail::impl_isoformat(as_timeline(*this)); }
std::string TimelineBase::to_string() const { return format("%Y-%m-%d %H:%M:%S"); }

pluss::codem::Series TimelineBase::to_Series() const { return ticks_; }

Timeline TimelineBase::add_years(int years) const {
  return detail::impl_add_offset(as_timeline(*this), Years{years});
}

Timeline TimelineBase::add_months(int months) const {
  return detail::impl_add_offset(as_timeline(*this), Months{months});
}

Timeline TimelineBase::add_days(int days) const {
  return detail::impl_add_nanos(as_timeline(*this),
                                static_cast<std::int64_t>(days) * 86'400'000'000'000LL);
}

Timeline TimelineBase::add_hours(int hours_) const {
  return detail::impl_add_nanos(as_timeline(*this),
                                static_cast<std::int64_t>(hours_) * 3'600'000'000'000LL);
}

Timeline TimelineBase::add_minutes(int minutes_) const {
  return detail::impl_add_nanos(as_timeline(*this),
                                static_cast<std::int64_t>(minutes_) * 60'000'000'000LL);
}

Timeline TimelineBase::add_seconds(int seconds_) const {
  return detail::impl_add_nanos(as_timeline(*this),
                                static_cast<std::int64_t>(seconds_) * 1'000'000'000LL);
}

int TimelineBase::years_between(const Timeline& other) const {
  CODEM_CHECK(dim() == 0 && other.dim() == 0, "years_between: scalar-only");
  const int y1 = year_scalar();
  const int m1 = month_scalar();
  const int d1 = day_scalar();
  const int y2 = scalar_field(other, "year");
  const int m2 = scalar_field(other, "month");
  const int d2 = scalar_field(other, "day");
  int years = y2 - y1;
  if (m2 < m1 || (m2 == m1 && d2 < d1)) {
    --years;
  }
  return years;
}

int TimelineBase::months_between(const Timeline& other) const {
  CODEM_CHECK(dim() == 0 && other.dim() == 0, "months_between: scalar-only");
  const int y1 = year_scalar();
  const int m1 = month_scalar();
  const int d1 = day_scalar();
  const int y2 = scalar_field(other, "year");
  const int m2 = scalar_field(other, "month");
  const int d2 = scalar_field(other, "day");
  int months = (y2 - y1) * 12 + (m2 - m1);
  if (d2 < d1) {
    --months;
  }
  return months;
}

int TimelineBase::days_between(const Timeline& other) const {
  CODEM_CHECK(dim() == 0 && other.dim() == 0, "days_between: scalar-only");
  const auto a = scalar_tick_utc(as_timeline(*this));
  const auto b = scalar_tick_utc(other);
  return static_cast<int>((b - a) / 86'400'000'000'000LL);
}

int TimelineBase::hours_between(const Timeline& other) const {
  CODEM_CHECK(dim() == 0 && other.dim() == 0, "hours_between: scalar-only");
  const auto a = scalar_tick_utc(as_timeline(*this));
  const auto b = scalar_tick_utc(other);
  return static_cast<int>((b - a) / 3'600'000'000'000LL);
}

int TimelineBase::minutes_between(const Timeline& other) const {
  CODEM_CHECK(dim() == 0 && other.dim() == 0, "minutes_between: scalar-only");
  const auto a = scalar_tick_utc(as_timeline(*this));
  const auto b = scalar_tick_utc(other);
  return static_cast<int>((b - a) / 60'000'000'000LL);
}

int TimelineBase::seconds_between(const Timeline& other) const {
  CODEM_CHECK(dim() == 0 && other.dim() == 0, "seconds_between: scalar-only");
  const auto a = scalar_tick_utc(as_timeline(*this));
  const auto b = scalar_tick_utc(other);
  return static_cast<int>((b - a) / 1'000'000'000LL);
}

Timeline TimelineBase::concat(const Timeline& other) const {
  const auto& self = as_timeline(*this);
  CODEM_CHECK(std::string_view(self.tz()) == std::string_view(other.tz()),
              "Timeline::concat: timezone mismatch");
  auto out = detail::impl_to_epoch_nanos_flat(self);
  auto rhs = detail::impl_to_epoch_nanos_flat(other);
  out.insert(out.end(), rhs.begin(), rhs.end());
  const std::array<std::int64_t, 1> shape{
      static_cast<std::int64_t>(out.size())};
  return detail::impl_tz_convert(ops_internal::timeline_from_epoch_nanos(out, shape),
                                 self.tz());
}

Timeline TimelineBase::concat(const std::vector<Timeline>& others) const {
  const auto& self = as_timeline(*this);
  std::vector<std::int64_t> out = detail::impl_to_epoch_nanos_flat(self);
  std::size_t reserve_extra = 0;
  for (const auto& t : others) {
    CODEM_CHECK(std::string_view(self.tz()) == std::string_view(t.tz()),
                "Timeline::concat: timezone mismatch");
    reserve_extra += static_cast<std::size_t>(t.numel());
  }
  out.reserve(out.size() + reserve_extra);
  for (const auto& t : others) {
    auto rhs = detail::impl_to_epoch_nanos_flat(t);
    out.insert(out.end(), rhs.begin(), rhs.end());
  }
  const std::array<std::int64_t, 1> shape{
      static_cast<std::int64_t>(out.size())};
  return detail::impl_tz_convert(ops_internal::timeline_from_epoch_nanos(out, shape),
                                 self.tz());
}

Timeline TimelineBase::repeat(std::int64_t times) const {
  const auto& self = as_timeline(*this);
  if (times <= 0) {
    return empty_like();
  }
  if (times == 1) {
    return self;
  }

  const auto flat = detail::impl_to_epoch_nanos_flat(self);
  std::vector<std::int64_t> out;
  out.reserve(flat.size() * static_cast<std::size_t>(times));
  for (std::int64_t i = 0; i < times; ++i) {
    out.insert(out.end(), flat.begin(), flat.end());
  }
  const std::array<std::int64_t, 1> shape{
      static_cast<std::int64_t>(out.size())};
  return detail::impl_tz_convert(ops_internal::timeline_from_epoch_nanos(out, shape),
                                 self.tz());
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
  return detail::impl_tz_convert(ops_internal::timeline_from_epoch_nanos(flat, shape),
                                 self.tz());
}

Timeline TimelineBase::fill_like(int year, int month, int day) const {
  const auto& self = as_timeline(*this);
  Timeline scalar = createFromDateTimeInts(year, month, day, 0, 0, 0);
  scalar = detail::impl_tz_convert(scalar, self.tz());
  const auto fill_tick = detail::impl_to_epoch_nanos_flat(scalar)[0];

  std::vector<std::int64_t> shape(self.sizes().begin(), self.sizes().end());
  const std::size_t n = static_cast<std::size_t>(self.numel());
  std::vector<std::int64_t> flat(n, fill_tick);
  return detail::impl_tz_convert(ops_internal::timeline_from_epoch_nanos(flat, shape),
                                 self.tz());
}

Timeline TimelineBase::fill_like(const std::string& date_str) const {
  const std::array<std::string_view, 1> sv{date_str};
  const auto parsed = ops_internal::timeline_from_utf8(sv, {});
  return fill_like(scalar_field(parsed, "year"),
                   scalar_field(parsed, "month"),
                   scalar_field(parsed, "day"));
}

bool TimelineBase::operator==(const Timeline& other) const {
  return all_true_bool_series(detail::impl_cmp(as_timeline(*this), other, "eq"),
                              "Timeline::operator==");
}

bool TimelineBase::operator!=(const Timeline& other) const { return !(*this == other); }

bool TimelineBase::operator<(const Timeline& other) const {
  return all_true_bool_series(detail::impl_cmp(as_timeline(*this), other, "lt"),
                              "Timeline::operator<");
}

bool TimelineBase::operator<=(const Timeline& other) const {
  return all_true_bool_series(detail::impl_cmp(as_timeline(*this), other, "le"),
                              "Timeline::operator<=");
}

bool TimelineBase::operator>(const Timeline& other) const {
  return all_true_bool_series(detail::impl_cmp(as_timeline(*this), other, "gt"),
                              "Timeline::operator>");
}

bool TimelineBase::operator>=(const Timeline& other) const {
  return all_true_bool_series(detail::impl_cmp(as_timeline(*this), other, "ge"),
                              "Timeline::operator>=");
}

Timeline TimelineBase::shift(int periods, std::string_view freq) const {
  const auto f = lower_ascii(freq);
  if (f == "y" || f == "yr" || f == "year" || f == "years" || f == "annual") {
    return add_years(periods);
  }
  if (f == "mo" || f == "mth" || f == "month" || f == "months") {
    return add_months(periods);
  }
  if (f == "d" || f == "day" || f == "days") {
    return add_days(periods);
  }
  if (f == "h" || f == "hr" || f == "hour" || f == "hours") {
    return add_hours(periods);
  }
  if (f == "t" || f == "min" || f == "minute" || f == "minutes") {
    return add_minutes(periods);
  }
  if (f == "s" || f == "sec" || f == "second" || f == "seconds") {
    return add_seconds(periods);
  }
  CODEM_FAIL("Timeline::shift: unsupported frequency");
}

Timeline TimelineBase::to_period(std::string_view freq) const {
  const auto& self = as_timeline(*this);
  const auto f = lower_ascii(freq);
  if (f == "y" || f == "yr" || f == "year" || f == "years" || f == "annual") {
    return Timeline::from_ymdhms(
        detail::impl_dt_field(self, "year"),
        full_i64_like(self, 1),
        full_i64_like(self, 1),
        full_i64_like(self, 0),
        full_i64_like(self, 0),
        full_i64_like(self, 0),
        self.tz());
  }
  if (f == "mo" || f == "mth" || f == "month" || f == "months") {
    return Timeline::from_ymdhms(
        detail::impl_dt_field(self, "year"),
        detail::impl_dt_field(self, "month"),
        full_i64_like(self, 1),
        full_i64_like(self, 0),
        full_i64_like(self, 0),
        full_i64_like(self, 0),
        self.tz());
  }
  if (f == "d" || f == "day" || f == "days") {
    return detail::impl_dt_round(self, "floor", "D");
  }
  if (f == "h" || f == "hr" || f == "hour" || f == "hours") {
    return detail::impl_dt_round(self, "floor", "H");
  }
  if (f == "t" || f == "min" || f == "minute" || f == "minutes") {
    return detail::impl_dt_round(self, "floor", "min");
  }
  if (f == "s" || f == "sec" || f == "second" || f == "seconds") {
    return detail::impl_dt_round(self, "floor", "S");
  }
  CODEM_FAIL("Timeline::to_period: unsupported frequency");
}

TimeIndex TimelineBase::to_timeindex(bool require_sorted) const {
  return ::pluss::codeh::_compat::invoke<TimeIndex>("make_timeindex",
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
    if (!ymd.ok()) {
      continue;
    }
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
    if (hhv < 0 || hhv > 23 || mmn < 0 || mmn > 59 || ssv < 0 || ssv > 59) {
      continue;
    }
    const auto ymd = std::chrono::year(static_cast<int>(yy))
                   / std::chrono::month(static_cast<unsigned>(mmv))
                   / std::chrono::day(static_cast<unsigned>(dd));
    if (!ymd.ok()) {
      continue;
    }
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

Timeline TimelineBase::from_epoch_seconds(const pluss::codem::Series& epoch_seconds,
                                          std::string_view tz_name) {
  const auto c = load_component_i64(epoch_seconds, "Timeline::from_epoch_seconds");
  std::vector<std::int64_t> flat(c.flat.size(), Timeline::kNaT);
  for (std::size_t i = 0; i < flat.size(); ++i) {
    if (c.flat[i] != Timeline::kNaT) {
      flat[i] = c.flat[i] * 1'000'000'000LL;
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
      flat[i] = c.flat[i] * 1'000'000LL;
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
      flat[i] = c.flat[i] * 1'000LL;
    }
  }
  Timeline t = ops_internal::timeline_from_epoch_nanos(flat, c.shape);
  return detail::impl_tz_convert(t, tz_name);
}

Timeline TimelineBase::now() {
  const auto ns = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
  return scalar_from_utc_ns(ns, "UTC");
}

Timeline TimelineBase::today() { return now().to_period("D"); }
Timeline TimelineBase::yesterday() { return today().add_days(-1); }
Timeline TimelineBase::tomorrow() { return today().add_days(1); }

Timeline TimelineBase::fromTimestamp(std::int64_t timestamp_seconds) {
  return scalar_from_utc_ns(timestamp_seconds * 1'000'000'000LL, "UTC");
}

Timeline TimelineBase::fromSeries(const pluss::codem::Series& series) {
  CODEM_CHECK(series.defined(), "Timeline::fromSeries: undefined input");
  const auto s = (series.scalar_type() == pluss::codem::ScalarType::Int64)
      ? series
      : series.to(pluss::codem::ScalarType::Int64);
  const auto c = s.contiguous();
  std::vector<std::int64_t> flat(static_cast<std::size_t>(c.numel()));
  if (!flat.empty()) {
    std::memcpy(flat.data(), c.raw_data(), flat.size() * sizeof(std::int64_t));
  }
  std::vector<std::int64_t> shape(s.sizes().begin(), s.sizes().end());
  return ops_internal::timeline_from_epoch_nanos(flat, shape);
}

Timeline TimelineBase::createFromDateInts(int year, int month, int day) {
  return createFromDateTimeInts(year, month, day, 0, 0, 0);
}

Timeline TimelineBase::createFromTimeInts(int hour, int minute, int second) {
  return createFromDateTimeInts(1970, 1, 1, hour, minute, second);
}

Timeline TimelineBase::createFromDateTimeInts(int year, int month, int day,
                                              int hour, int minute, int second) {
  std::ostringstream oss;
  oss << std::setw(4) << std::setfill('0') << year << "-"
      << std::setw(2) << std::setfill('0') << month << "-"
      << std::setw(2) << std::setfill('0') << day << "T"
      << std::setw(2) << std::setfill('0') << hour << ":"
      << std::setw(2) << std::setfill('0') << minute << ":"
      << std::setw(2) << std::setfill('0') << second;
  const std::string iso = oss.str();
  const std::array<std::string_view, 1> flat{iso};
  return ops_internal::timeline_from_utf8(flat, {});
}

std::vector<Timeline> TimelineBase::date_range(const Timeline& start,
                                               const Timeline& end,
                                               std::string_view freq,
                                               int step) {
  CODEM_CHECK(step != 0, "Timeline::date_range: step must be non-zero");
  CODEM_CHECK(start.dim() == 0 && end.dim() == 0,
              "Timeline::date_range: scalar start/end required");
  std::vector<Timeline> out;
  Timeline cur = start.to_period(freq);
  const Timeline endp = end.to_period(freq);
  if (step > 0) {
    while (cur <= endp) {
      out.push_back(cur);
      cur = cur.shift(step, freq);
    }
  } else {
    while (cur >= endp) {
      out.push_back(cur);
      cur = cur.shift(step, freq);
    }
  }
  return out;
}

} // namespace pluss::codeh
