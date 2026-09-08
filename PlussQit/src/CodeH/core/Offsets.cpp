#include "CodeH/core/Offsets.h"

#include <chrono>
#include <limits>
#include <memory>
#include <string>

#include "CodeH/core/Timeline.h"
#include "CodeH/core/TimeZone.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesIterator.h"

namespace pluss::codeh {

using namespace std::chrono;

static void split_local(std::int64_t local_ns, int& y, unsigned& mo, unsigned& d,
                        int& hh, int& mm, int& ss, int& nsec) {
  const auto tp = sys_time<nanoseconds>(nanoseconds(local_ns));
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

static std::int64_t checked_i64(__int128 value, const char* where) {
  CODEM_CHECK(value > std::numeric_limits<std::int64_t>::min() &&
                  value <= std::numeric_limits<std::int64_t>::max(),
              std::string(where) + ": result is outside Timeline range");
  return static_cast<std::int64_t>(value);
}

static std::int64_t local_nanos(std::int64_t utc_ns, int offset_minutes,
                                const char* where) {
  constexpr __int128 kNanosPerMinute = 60LL * 1'000'000'000LL;
  return checked_i64(static_cast<__int128>(utc_ns) +
                         static_cast<__int128>(offset_minutes) * kNanosPerMinute,
                     where);
}

static std::int64_t pack_local(int y, unsigned mo, unsigned d, int hh, int mm,
                               int ss, int nsec, const char* where) {
  const auto ymd = year{y}/month{mo}/day{d};
  CODEM_CHECK(ymd.ok(), std::string(where) + ": invalid calendar result");
  constexpr __int128 kNanosPerDay = 86'400LL * 1'000'000'000LL;
  constexpr __int128 kNanosPerSecond = 1'000'000'000LL;
  const auto day_count = sys_days(ymd).time_since_epoch().count();
  const __int128 value = static_cast<__int128>(day_count) * kNanosPerDay +
      static_cast<__int128>(hh * 3600 + mm * 60 + ss) * kNanosPerSecond + nsec;
  return checked_i64(value, where);
}

static unsigned days_in_month(int y, unsigned mo) {
  const auto first = sys_days(year{y}/month{mo}/day{1});
  const auto next = (mo == 12) ? sys_days(year{y+1}/month{1}/day{1})
                               : sys_days(year{y}/month{mo+1}/day{1});
  return (unsigned)(next - first).count();
}

static bool is_weekend(int y, unsigned mo, unsigned d) {
  const auto dp = sys_days(year{y}/month{mo}/day{d});
  const int wd = (int)weekday{dp}.c_encoding(); // 0=Sun
  return wd == 0 || wd == 6;
}

static std::int64_t apply_months(std::int64_t utc_epoch_nanos,
                                 const char* tz_name, std::int64_t count,
                                 const char* where) {
  const auto tz = TimeZone::load(tz_name ? tz_name : "UTC");
  const int off = tz.offset_minutes_at_utc(utc_epoch_nanos);
  const std::int64_t local_ns = local_nanos(utc_epoch_nanos, off, where);

  int y, hh, mm, ss, nsec;
  unsigned mo, d;
  split_local(local_ns, y, mo, d, hh, mm, ss, nsec);

  const std::int64_t total = static_cast<std::int64_t>(mo) - 1 + count;
  std::int64_t target_year = static_cast<std::int64_t>(y) + total / 12;
  std::int64_t new_mo0 = total % 12;
  if (new_mo0 < 0) { new_mo0 += 12; --target_year; }
  CODEM_CHECK(target_year >= static_cast<int>(year::min()) &&
                  target_year <= static_cast<int>(year::max()),
              std::string(where) + ": year is outside the supported range");
  y = static_cast<int>(target_year);
  unsigned new_mo = (unsigned)(new_mo0 + 1);
  unsigned dim = days_in_month(y, new_mo);
  unsigned new_d = d > dim ? dim : d;

  const std::int64_t new_local =
      pack_local(y, new_mo, new_d, hh, mm, ss, nsec, where);
  return tz.local_to_utc(new_local, /*fold=*/0, /*nonexistent=*/"raise");
}

std::int64_t Months::apply_utc(std::int64_t utc_epoch_nanos, const char* tz_name) const {
  return apply_months(utc_epoch_nanos, tz_name, n, "Timeline::add_months");
}

std::int64_t Years::apply_utc(std::int64_t utc_epoch_nanos, const char* tz_name) const {
  return apply_months(utc_epoch_nanos, tz_name,
                      static_cast<std::int64_t>(n) * 12,
                      "Timeline::add_years");
}

std::int64_t Days::apply_utc(std::int64_t utc_epoch_nanos, const char* tz_name) const {
  const auto tz = TimeZone::load(tz_name ? tz_name : "UTC");
  const int off = tz.offset_minutes_at_utc(utc_epoch_nanos);
  const auto local_ns = local_nanos(utc_epoch_nanos, off, "Timeline::add_days");
  constexpr __int128 kNanosPerDay = 86'400LL * 1'000'000'000LL;
  const auto target_local = checked_i64(
      static_cast<__int128>(local_ns) + static_cast<__int128>(n) * kNanosPerDay,
      "Timeline::add_days");
  return tz.local_to_utc(target_local, 0, "raise");
}

std::int64_t BusinessDays::apply_utc(std::int64_t utc_epoch_nanos, const char* tz_name) const {
  const auto tz = TimeZone::load(tz_name ? tz_name : "UTC");
  const int off = tz.offset_minutes_at_utc(utc_epoch_nanos);
  std::int64_t local_ns = utc_epoch_nanos + (std::int64_t)off * 60LL * 1'000'000'000LL;

  int y, hh, mm, ss, nsec;
  unsigned mo, d;
  split_local(local_ns, y, mo, d, hh, mm, ss, nsec);

  int remaining = n;
  int dir = (remaining >= 0) ? 1 : -1;
  while (remaining != 0) {
    // move one day
    const auto dp = sys_days(year{y}/month{mo}/day{d}) + days(dir);
    const auto ymd = year_month_day(dp);
    y = int(ymd.year());
    mo = unsigned(ymd.month());
    d = unsigned(ymd.day());
    if (!is_weekend(y, mo, d)) remaining -= dir;
  }

  const std::int64_t new_local = pack_local(
      y, mo, d, hh, mm, ss, nsec, "Timeline::add_business_days");
  return tz.local_to_utc(new_local, 0, "shift_forward");
}

std::int64_t MonthEnd::apply_utc(std::int64_t utc_epoch_nanos, const char* tz_name) const {
  const auto tz = TimeZone::load(tz_name ? tz_name : "UTC");
  const int off = tz.offset_minutes_at_utc(utc_epoch_nanos);
  const std::int64_t local_ns = utc_epoch_nanos + (std::int64_t)off * 60LL * 1'000'000'000LL;

  int y, hh, mm, ss, nsec;
  unsigned mo, d;
  split_local(local_ns, y, mo, d, hh, mm, ss, nsec);

  // Move by n months first.
  Months m(n);
  const auto moved_utc = m.apply_utc(utc_epoch_nanos, tz_name);
  const int off2 = tz.offset_minutes_at_utc(moved_utc);
  const std::int64_t moved_local = moved_utc + (std::int64_t)off2 * 60LL * 1'000'000'000LL;
  split_local(moved_local, y, mo, d, hh, mm, ss, nsec);

  const unsigned dim = days_in_month(y, mo);
  const std::int64_t new_local = pack_local(
      y, mo, dim, hh, mm, ss, nsec, "Timeline::add_month_end");
  return tz.local_to_utc(new_local, 0, "shift_forward");
}

namespace detail {

static void check_timeline_core_(const Timeline& t) {
  TimelineBase::validate_ticks_series(t.unsafe_ticks());
}

static std::unique_ptr<CalendarOffset> negate_offset_(const CalendarOffset& off) {
  if (const auto* p = dynamic_cast<const Months*>(&off)) {
    return std::make_unique<Months>(-p->n);
  }
  if (const auto* p = dynamic_cast<const Years*>(&off)) {
    return std::make_unique<Years>(-p->n);
  }
  if (const auto* p = dynamic_cast<const Days*>(&off)) {
    return std::make_unique<Days>(-p->n);
  }
  if (const auto* p = dynamic_cast<const BusinessDays*>(&off)) {
    return std::make_unique<BusinessDays>(-p->n);
  }
  if (const auto* p = dynamic_cast<const MonthEnd*>(&off)) {
    return std::make_unique<MonthEnd>(-p->n);
  }
  CODEM_FAIL("Timeline::sub_offset: unsupported CalendarOffset subtype");
}

Timeline impl_add_offset(const Timeline& a, const CalendarOffset& off) {
  check_timeline_core_(a);
  auto out = pluss::codem::Series::empty(a.sizes(), pluss::codem::ScalarType::Int64);
  if (a.numel() == 0) {
    return Timeline(std::move(out), a.unsafe_tz_name());
  }
  auto it = pluss::codem::SeriesIterator::unary(a.unsafe_ticks(), out);
  const auto* in = reinterpret_cast<const std::int64_t*>(it.a().raw_data());
  auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());
  const char* tz_name = a.unsafe_tz_name().c_str();
  it.for_each_index([&](std::int64_t, std::int64_t input_offset,
                        std::int64_t, std::int64_t output_offset) {
    const auto value = in[input_offset];
    o[output_offset] = value == Timeline::kNaT
        ? Timeline::kNaT
        : off.apply_utc(value, tz_name);
  });
  return Timeline(std::move(out), a.unsafe_tz_name());
}

Timeline impl_sub_offset(const Timeline& a, const CalendarOffset& off) {
  auto neg = negate_offset_(off);
  return impl_add_offset(a, *neg);
}

} // namespace detail

} // namespace pluss::codeh
