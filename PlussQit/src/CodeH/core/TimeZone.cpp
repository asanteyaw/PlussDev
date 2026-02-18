#include "CodeH/core/TimeZone.h"

#include <chrono>
#include <cctype>
#include <string>

#include "CodeH/core/Errors.h"

namespace pluss::codeh {

using namespace std::chrono;

static std::string lower(std::string_view s) {
  std::string out; out.reserve(s.size());
  for (char c : s) out.push_back((char)std::tolower((unsigned char)c));
  return out;
}

static int weekday_of_ymd(int y, unsigned m, unsigned d) {
  // 0=Sunday..6=Saturday
  const auto dp = sys_days(year{y}/month{m}/day{d});
  const auto wd = weekday{dp};
  return (int)wd.c_encoding();
}

static unsigned nth_weekday_of_month(int y, unsigned m, int weekday_sun0, unsigned n) {
  // Return day-of-month for the nth occurrence of weekday in month.
  // weekday_sun0: 0=Sunday..6=Saturday.
  // n >= 1.
  CODEM_CHECK(n >= 1, "nth_weekday_of_month: n must be >= 1");
  int w1 = weekday_of_ymd(y, m, 1);
  int delta = (weekday_sun0 - w1 + 7) % 7;
  unsigned dom = 1u + (unsigned)delta + 7u * (n - 1u);
  return dom;
}

// DST rules for America/Edmonton (Mountain):
// - Standard offset: -07:00
// - DST offset:      -06:00
// - DST starts: second Sunday in March at 02:00 local standard time
// - DST ends:   first Sunday in November at 02:00 local DST time
// These rules match modern North America (2007+). We apply them for all years.
struct EdmontonRules {
  static constexpr int kStd = -7 * 60;
  static constexpr int kDst = -6 * 60;

  static sys_time<nanoseconds> dst_start_utc(int y) {
    // local standard time 02:00 -> UTC = local - offset
    const unsigned dom = nth_weekday_of_month(y, 3, 0, 2); // 2nd Sunday March
    const auto local = sys_days(year{y}/month{3}/day{dom}) + hours(2);
    return sys_time<nanoseconds>(local.time_since_epoch()) - minutes(kStd);
  }

  static sys_time<nanoseconds> dst_end_utc(int y) {
    // local DST time 02:00 -> UTC = local - offset
    const unsigned dom = nth_weekday_of_month(y, 11, 0, 1); // 1st Sunday Nov
    const auto local = sys_days(year{y}/month{11}/day{dom}) + hours(2);
    return sys_time<nanoseconds>(local.time_since_epoch()) - minutes(kDst);
  }

  static int offset_minutes_at_utc(std::int64_t utc_ns) {
    const auto tp = sys_time<nanoseconds>(nanoseconds(utc_ns));
    const auto dp = floor<days>(tp);
    const auto ymd = year_month_day(dp);
    const int y = int(ymd.year());
    const auto s = dst_start_utc(y);
    const auto e = dst_end_utc(y);
    // DST active in [s, e)
    return (tp >= s && tp < e) ? kDst : kStd;
  }

  static std::int64_t local_to_utc(std::int64_t local_ns, int fold, std::string_view nonexistent) {
    const auto local_tp = sys_time<nanoseconds>(nanoseconds(local_ns));
    const auto dp = floor<days>(local_tp);
    const auto ymd = year_month_day(dp);
    const int y = int(ymd.year());

    // Build transition instants in local time.
    const unsigned start_dom = nth_weekday_of_month(y, 3, 0, 2);
    const unsigned end_dom   = nth_weekday_of_month(y, 11, 0, 1);
    const auto dst_start_local = sys_days(year{y}/month{3}/day{start_dom}) + hours(2);
    const auto dst_end_local   = sys_days(year{y}/month{11}/day{end_dom}) + hours(2);

    // Spring forward gap: [02:00, 03:00) local on start day.
    const auto gap_begin = dst_start_local;
    const auto gap_end   = dst_start_local + hours(1);
    if (local_tp >= gap_begin && local_tp < gap_end) {
      if (nonexistent == "shift_forward") {
        const auto shifted = gap_end;
        return (sys_time<nanoseconds>(shifted.time_since_epoch()) - minutes(kDst)).time_since_epoch().count();
      }
      CODEM_FAIL("tz_localize: nonexistent local time (DST spring-forward gap)");
    }

    // Fall back overlap: [01:00, 02:00) local on end day.
    const auto overlap_begin = dst_end_local - hours(1);
    const auto overlap_end   = dst_end_local;
    if (local_tp >= overlap_begin && local_tp < overlap_end) {
      const int off = (fold == 0) ? kDst : kStd;
      return (local_tp - minutes(off)).time_since_epoch().count();
    }

    // Otherwise: determine whether local time is in DST period.
    // Local time after gap_end is in DST until overlap_begin.
    const bool in_dst = (local_tp >= gap_end && local_tp < overlap_begin);
    const int off = in_dst ? kDst : kStd;
    return (local_tp - minutes(off)).time_since_epoch().count();
  }
};

TimeZone TimeZone::load(std::string_view name) {
  const auto n = std::string(name);
  if (n.empty() || n == "UTC" || n == "utc") return TimeZone("UTC");
  if (n == "America/Edmonton" || n == "america/edmonton") return TimeZone("America/Edmonton");
  CODEM_FAIL("Unknown timezone: " + n + " (currently supported: UTC, America/Edmonton)");
}

int TimeZone::offset_minutes_at_utc(std::int64_t utc_epoch_nanos) const {
  if (name_ == "UTC") return 0;
  if (name_ == "America/Edmonton") return EdmontonRules::offset_minutes_at_utc(utc_epoch_nanos);
  return 0;
}

std::int64_t TimeZone::local_to_utc(std::int64_t local_epoch_nanos, int fold, std::string_view nonexistent) const {
  if (name_ == "UTC") return local_epoch_nanos;
  if (name_ == "America/Edmonton") return EdmontonRules::local_to_utc(local_epoch_nanos, fold, nonexistent);
  return local_epoch_nanos;
}

} // namespace pluss::codeh
