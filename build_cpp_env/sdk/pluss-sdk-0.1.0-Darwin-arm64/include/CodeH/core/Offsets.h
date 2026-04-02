#pragma once

#include <cstdint>

namespace pluss::codeh {

// Calendar-aware offsets (month/year/business day) applied to Timeline instants.
// Offsets operate in *local civil time* in the Timeline's tz (if any), then
// return an instant that preserves the wall-clock time when possible.

struct CalendarOffset {
  virtual ~CalendarOffset() = default;
  virtual std::int64_t apply_utc(std::int64_t utc_epoch_nanos, const char* tz_name) const = 0;
};

struct Months : CalendarOffset {
  explicit Months(int n) : n(n) {}
  int n;
  std::int64_t apply_utc(std::int64_t utc_epoch_nanos, const char* tz_name) const override;
};

struct Years : CalendarOffset {
  explicit Years(int n) : n(n) {}
  int n;
  std::int64_t apply_utc(std::int64_t utc_epoch_nanos, const char* tz_name) const override;
};

struct BusinessDays : CalendarOffset {
  explicit BusinessDays(int n) : n(n) {}
  int n;
  std::int64_t apply_utc(std::int64_t utc_epoch_nanos, const char* tz_name) const override;
};

struct MonthEnd : CalendarOffset {
  explicit MonthEnd(int n) : n(n) {}
  int n; // number of month ends forward (negative allowed)
  std::int64_t apply_utc(std::int64_t utc_epoch_nanos, const char* tz_name) const override;
};

} // namespace pluss::codeh
