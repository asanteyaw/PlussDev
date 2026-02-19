#pragma once

#include "pluss/export.h"

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "CodeH/core/Errors.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/Series.h"

namespace pluss::codeh {

class Timeline;
class TimeIndex;

// Lightweight Timeline handle base (similar to CodeM::SeriesBase):
// - cheap copy (Series is ref-counted)
// - shared core invariants for all Timeline-like types
class CODEH_API TimelineBase {
public:
  TimelineBase() = delete;
  TimelineBase(const TimelineBase&) = default;
  TimelineBase(TimelineBase&&) noexcept = default;
  TimelineBase& operator=(const TimelineBase&) = default;
  TimelineBase& operator=(TimelineBase&&) noexcept = default;
  ~TimelineBase() = default;

  bool defined() const noexcept { return ticks_.defined(); }
  std::span<const std::int64_t> sizes() const { return ticks_.sizes(); }
  std::int64_t dim() const { return ticks_.dim(); }
  std::int64_t numel() const { return ticks_.numel(); }
  bool is_scalar() const { return dim() == 0; }

  // Time zone metadata (pandas-like): ticks_ always represent UTC instants.
  std::string_view tz() const noexcept { return tz_name_; }

  // Internal escape hatches for compat/dispatch layers.
  const pluss::codem::Series& unsafe_ticks() const noexcept { return ticks_; }
  pluss::codem::Series& unsafe_ticks() noexcept { return ticks_; }
  const std::string& unsafe_tz_name() const noexcept { return tz_name_; }
  std::string& unsafe_tz_name() noexcept { return tz_name_; }

  // Materialize epoch nanos in row-major logical order.
  template <std::output_iterator<std::int64_t> OutIt>
  void copy_epoch_nanos_to(OutIt out) const {
    CODEM_CHECK(defined(), "Timeline: undefined");
    validate_ticks_series(ticks_);

    if (ticks_.numel() == 0) {
      return;
    }

    if (ticks_.is_contiguous()) {
      const auto* p = reinterpret_cast<const std::int64_t*>(ticks_.raw_data());
      std::copy_n(p, static_cast<std::size_t>(ticks_.numel()), out);
      return;
    }

    const auto contig = ticks_.contiguous();
    const auto* p = reinterpret_cast<const std::int64_t*>(contig.raw_data());
    std::copy_n(p, static_cast<std::size_t>(contig.numel()), out);
  }

  std::vector<std::int64_t> to_epoch_nanos_vector() const {
    CODEM_CHECK(defined(), "Timeline: undefined");
    std::vector<std::int64_t> out;
    out.reserve(static_cast<std::size_t>(numel()));
    copy_epoch_nanos_to(std::back_inserter(out));
    return out;
  }

  // -----------------------------------------------------------------------
  // Legacy-style convenience layer (inherited by Timeline).
  // Definitions are in Timeline.cpp to keep this header lightweight and to
  // avoid include cycles with Timeline/Duration internals.
  // -----------------------------------------------------------------------

  // Old API compatibility: Timeline in CodeH always has both date/time parts
  // when defined (stored as UTC epoch nanos).
  bool has_date() const noexcept;
  bool has_time() const noexcept;

  // Scalar field extraction helpers.
  int year_scalar() const;
  int month_scalar() const;
  int day_scalar() const;
  int hour_scalar() const;
  int minute_scalar() const;
  int second_scalar() const;
  int day_of_week_scalar() const; // 0=Sunday..6=Saturday
  int day_of_year_scalar() const;
  int quarter_scalar() const;
  int week_of_year_scalar() const;

  bool is_leap_year_scalar() const;
  bool is_month_start_scalar() const;
  bool is_month_end_scalar() const;
  bool is_quarter_start_scalar() const;
  bool is_quarter_end_scalar() const;
  bool is_weekend_scalar() const;

  std::string weekday_name(bool abbreviated = false) const;
  std::string month_name(bool abbreviated = false) const;

  // Immutable "setters" for compatibility with the old Timeline API.
  Timeline set_year(int year) const;
  Timeline set_month(int month) const;
  Timeline set_day(int day) const;
  Timeline set_hour(int hour) const;
  Timeline set_minute(int minute) const;
  Timeline set_second(int second) const;

  std::string format(const std::string& format_str) const;
  std::string toISOString() const;
  std::string to_string() const;

  pluss::codem::Series to_Series() const;

  Timeline add_years(int years) const;
  Timeline add_months(int months) const;
  Timeline add_days(int days) const;
  Timeline add_hours(int hours) const;
  Timeline add_minutes(int minutes) const;
  Timeline add_seconds(int seconds) const;

  int years_between(const Timeline& other) const;
  int months_between(const Timeline& other) const;
  int days_between(const Timeline& other) const;
  int hours_between(const Timeline& other) const;
  int minutes_between(const Timeline& other) const;
  int seconds_between(const Timeline& other) const;

  Timeline concat(const Timeline& other) const;
  Timeline concat(const std::vector<Timeline>& others) const;
  Timeline repeat(std::int64_t times) const;
  Timeline empty_like() const;
  Timeline fill_like(int year, int month, int day) const;
  Timeline fill_like(const std::string& date_str) const;

  bool operator==(const Timeline& other) const;
  bool operator!=(const Timeline& other) const;
  bool operator<(const Timeline& other) const;
  bool operator<=(const Timeline& other) const;
  bool operator>(const Timeline& other) const;
  bool operator>=(const Timeline& other) const;

  Timeline shift(int periods, std::string_view freq) const;
  Timeline to_period(std::string_view freq) const;
  TimeIndex to_timeindex(bool require_sorted = true) const;

  // High-priority interop tranche for dataframe-style datetime workflows.
  // Build Timeline from calendar component columns (Series) and convert back
  // to epoch units used by pandas/NumPy tooling.
  static Timeline from_ymd(const pluss::codem::Series& year,
                           const pluss::codem::Series& month,
                           const pluss::codem::Series& day,
                           std::string_view tz_name = "UTC");
  static Timeline from_ymdhms(const pluss::codem::Series& year,
                              const pluss::codem::Series& month,
                              const pluss::codem::Series& day,
                              const pluss::codem::Series& hour,
                              const pluss::codem::Series& minute,
                              const pluss::codem::Series& second,
                              std::string_view tz_name = "UTC");

  pluss::codem::Series epoch_nanos() const;
  pluss::codem::Series epoch_micros() const;
  pluss::codem::Series epoch_millis() const;
  pluss::codem::Series epoch_seconds() const;

  static Timeline from_epoch_seconds(const pluss::codem::Series& epoch_seconds,
                                     std::string_view tz_name = "UTC");
  static Timeline from_epoch_millis(const pluss::codem::Series& epoch_millis,
                                    std::string_view tz_name = "UTC");
  static Timeline from_epoch_micros(const pluss::codem::Series& epoch_micros,
                                    std::string_view tz_name = "UTC");

  static Timeline now();
  static Timeline today();
  static Timeline yesterday();
  static Timeline tomorrow();
  static Timeline fromTimestamp(std::int64_t timestamp_seconds);
  static Timeline fromSeries(const pluss::codem::Series& series);
  static Timeline createFromDateInts(int year, int month, int day);
  static Timeline createFromTimeInts(int hour, int minute, int second);
  static Timeline createFromDateTimeInts(int year, int month, int day,
                                         int hour, int minute, int second);
  static std::vector<Timeline> date_range(const Timeline& start,
                                          const Timeline& end,
                                          std::string_view freq = "D",
                                          int step = 1);

  static void validate_ticks_series(const pluss::codem::Series& ticks) {
    CODEM_CHECK(ticks.defined(), "Timeline: underlying Series is undefined");
    CODEM_CHECK(ticks.scalar_type() == pluss::codem::ScalarType::Int64,
                "Timeline: ticks must be int64 epoch nanos");
    CODEM_CHECK(ticks.dim() >= 0 && ticks.dim() <= 3, "Timeline: only supports 0D..3D");
  }

protected:
  explicit TimelineBase(pluss::codem::Series ticks, std::string tz_name = "UTC")
      : ticks_(std::move(ticks)), tz_name_(std::move(tz_name)) {
    validate_ticks_series(ticks_);
  }

  void reset_core(pluss::codem::Series ticks, std::string tz_name = "UTC") {
    validate_ticks_series(ticks);
    ticks_ = std::move(ticks);
    tz_name_ = std::move(tz_name);
  }

  pluss::codem::Series ticks_;
  std::string tz_name_{"UTC"};
};

} // namespace pluss::codeh
