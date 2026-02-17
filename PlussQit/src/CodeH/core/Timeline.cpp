
#include "CodeH/core/Timeline.h"

#include <algorithm>
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
  CODEM_CHECK(s.defined(), "Timeline: underlying Series is undefined");
  CODEM_CHECK(s.scalar_type() == pluss::codem::ScalarType::Int64, "Timeline: ticks must be int64 epoch nanos");
  CODEM_CHECK(s.dim() >= 0 && s.dim() <= 3, "Timeline: only supports 0D..3D");
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
  // We don't have a direct Series<int64> factory that takes int64 flat; use make_from_doubles.
  // TODO(CodeM): add int64 factory. For now, go through doubles safely (int64 representable for epoch nanos range used here).
  std::vector<double> tmp;
  tmp.reserve(ticks_flat.size());
  for (auto v : ticks_flat) tmp.push_back((double)v);
  auto s = pluss::codem::Series::make_from_doubles(tmp, shape, pluss::codem::ScalarType::Int64);
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

  std::vector<std::int64_t> ticks;
  ticks.reserve(utf8_flat.size());
  for (auto sv : utf8_flat) {
    const auto p = parse_datetime(sv);
    CODEM_CHECK(p.ok, "moment: failed to parse datetime");
    ticks.push_back(to_epoch_nanos_utc(p));
  }
  return timeline_from_epoch_nanos(ticks, shape);
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
  auto* op = reinterpret_cast<std::int64_t*>(it.out().raw_mutable_data());
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
  auto* op = reinterpret_cast<std::int64_t*>(it.out().raw_mutable_data());
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
  auto* op = reinterpret_cast<std::int64_t*>(it.out().raw_mutable_data());
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
  auto* optr = reinterpret_cast<bool*>(it.out().raw_mutable_data());
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
    auto* op = reinterpret_cast<std::int64_t*>(it.out().raw_mutable_data());
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

  auto* op = reinterpret_cast<std::int32_t*>(it.out().raw_mutable_data());
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
  auto* op = reinterpret_cast<bool*>(it.out().raw_mutable_data());

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
  auto* op = reinterpret_cast<std::int64_t*>(it.out().raw_mutable_data());

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
  return Timeline(ops_internal::timeline_from_epoch_nanos(out, { (std::int64_t)out.size() }).ticks_.clone(), std::string(start.tz()));
}

Timeline impl_tz_convert(const Timeline& a, std::string_view tz_name) {
  check_timeline_core(a.ticks_);
  (void)TimeZone::load(tz_name); // validate
  return Timeline(a.ticks_.clone(), std::string(tz_name));
}

Timeline impl_tz_localize(const Timeline& a, std::string_view tz_name, int fold, std::string_view nonexistent) {
  // Interpret ticks_ as naive *local* wall times in tz_name, convert to UTC instants.
  // This implementation supports fixed offsets directly; transition tables are handled by TimeZone registry.
  (void)fold; (void)nonexistent; // reserved for DST folds/nonexistent handling
  check_timeline_core(a.ticks_);
  const auto tz = TimeZone::load(tz_name);

  auto out = pluss::codem::Series::empty(a.ticks_.sizes(), pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::unary(a.ticks_, out);
  const auto* ap = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(it.out().raw_mutable_data());

  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    const auto local_ns = ap[ao];
    if (local_ns == Timeline::kNaT) { op[oo] = Timeline::kNaT; return; }
    // Best-effort: use base offset for tz tables; true DST localization requires searching transitions (future patch).
    const int off = tz.base_offset_minutes();
    op[oo] = local_ns - (std::int64_t)off * 60LL * 1'000'000'000LL;
  });

  return Timeline(std::move(out), "UTC").tz_convert(tz_name);
}

Timeline impl_time_of_day(const Timeline& a) {
  // Return a Timeline where date is epoch day 1970-01-01 but time preserved in local tz.
  check_timeline_core(a.ticks_);
  const auto tz = TimeZone::load(a.tz());
  auto out = pluss::codem::Series::empty(a.ticks_.sizes(), pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::unary(a.ticks_, out);
  const auto* ap = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(it.out().raw_mutable_data());
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
  auto* op = reinterpret_cast<bool*>(it.out().raw_mutable_data());
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
  auto* op = reinterpret_cast<bool*>(it.out().raw_mutable_data());
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
  std::vector<std::int64_t> idx((size_t)n);
  for (std::int64_t i = 0; i < n; ++i) idx[(size_t)i] = i;
  const auto* x = reinterpret_cast<const std::int64_t*>(a.ticks_.raw_data());
  std::stable_sort(idx.begin(), idx.end(), [&](std::int64_t i, std::int64_t j) {
    const auto ai = x[i], aj = x[j];
    if (ai == Timeline::kNaT && aj == Timeline::kNaT) return i < j;
    if (ai == Timeline::kNaT) return false;
    if (aj == Timeline::kNaT) return true;
    return ai < aj;
  });
  // Build Series<int64>
  std::vector<double> tmp; tmp.reserve(idx.size());
  for (auto v : idx) tmp.push_back((double)v);
  auto out = pluss::codem::Series::make_from_doubles(tmp, {n}, pluss::codem::ScalarType::Int64);
  return out;
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
  return ops_internal::timeline_from_epoch_nanos(std::span<const std::int64_t>(&best, 1), {}).tz_convert(a.tz());
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
  return ops_internal::timeline_from_epoch_nanos(std::span<const std::int64_t>(&best, 1), {}).tz_convert(a.tz());
}

Timeline impl_sort(const Timeline& a) {
  check_timeline_core(a.ticks_);
  CODEM_CHECK(a.dim() == 1, "sort currently supports 1D only");
  std::vector<std::int64_t> x((size_t)a.numel());
  std::memcpy(x.data(), a.ticks_.raw_data(), (size_t)a.numel()*sizeof(std::int64_t));
  std::stable_sort(x.begin(), x.end(), [](std::int64_t u, std::int64_t v) {
    if (u == Timeline::kNaT && v == Timeline::kNaT) return false;
    if (u == Timeline::kNaT) return false;
    if (v == Timeline::kNaT) return true;
    return u < v;
  });
  return ops_internal::timeline_from_epoch_nanos(x, { (std::int64_t)x.size() }).tz_convert(a.tz());
}

Timeline impl_unique(const Timeline& a) {
  check_timeline_core(a.ticks_);
  CODEM_CHECK(a.dim() == 1, "unique currently supports 1D only");
  std::vector<std::int64_t> x((size_t)a.numel());
  std::memcpy(x.data(), a.ticks_.raw_data(), (size_t)a.numel()*sizeof(std::int64_t));
  std::stable_sort(x.begin(), x.end());
  x.erase(std::unique(x.begin(), x.end()), x.end());
  return ops_internal::timeline_from_epoch_nanos(x, { (std::int64_t)x.size() }).tz_convert(a.tz());
}

Timeline impl_fill_nat(const Timeline& a, const Timeline& value) {
  check_timeline_core(a.ticks_);
  check_timeline_core(value.ticks_);
  CODEM_CHECK(value.dim() == 0, "fill_nat value must be scalar Timeline");
  const auto v = reinterpret_cast<const std::int64_t*>(value.ticks_.raw_data())[0];
  auto out = pluss::codem::Series::empty(a.ticks_.sizes(), pluss::codem::ScalarType::Int64);
  auto it = pluss::codem::SeriesIterator::unary(a.ticks_, out);
  const auto* ap = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(it.out().raw_mutable_data());
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
  auto* op = reinterpret_cast<std::int64_t*>(it.out().raw_mutable_data());
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
  auto* op = reinterpret_cast<std::int64_t*>(it.out().raw_mutable_data());
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
  auto* op = reinterpret_cast<std::int64_t*>(it.out().raw_mutable_data());
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

} // namespace pluss::codeh
