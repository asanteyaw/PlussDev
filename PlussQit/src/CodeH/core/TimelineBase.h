#pragma once

#include "pluss/export.h"

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <iterator>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "CodeH/core/Errors.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

namespace pluss::codeh {

class Timeline;
class TimeIndex;

enum class TimelineSpanUnit : std::uint8_t {
  Nanos = 0,
  Micros = 1,
  Millis = 2,
  Seconds = 3,
  Days = 4,
  Weeks = 5,
  Months = 6,
  Years = 7,
  BusinessDays = 8,
  Minutes = 9,
  Hours = 10,
};

enum class TimelineSpanMode : std::uint8_t {
  Complete = 0, // completed elapsed calendar units
  Boundary = 1, // crossed calendar/unit labels
};

enum class TimelineSpanClosed : std::uint8_t {
  Right = 0,   // (start, end]
  Left = 1,    // [start, end)
  Both = 2,    // [start, end]
  Neither = 3, // (start, end)
};

enum class TimelineSpanAnchor : std::uint8_t {
  DayOfMonth = 0, // strict calendar-day completion
  MonthEnd = 1,   // treat last-day-of-month endpoints as equivalent
};

struct BusinessCalendar {
  static BusinessCalendar monfri();
  static BusinessCalendar usa_federal(int year_start, int year_end);
  static BusinessCalendar canada_federal(int year_start, int year_end);
  static BusinessCalendar uk_bank(int year_start, int year_end);
  static BusinessCalendar target2(int year_start, int year_end);
  static BusinessCalendar nyse(int year_start, int year_end);

  // Bits use std::chrono::weekday::c_encoding(): Sunday=0, Monday=1, ..., Saturday=6.
  // Default is Monday-Friday only. Holidays are local epoch-day labels in the
  // Timeline timezone, e.g. values produced by tl().epoch(TimeUnit::Days).
  std::uint8_t weekmask{0b0111110};
  std::vector<std::int64_t> holidays{};
  std::string name{"MonFri"};
  std::string region{};
  std::string timezone{};
};

struct TimelineSpanOptions {
  TimelineSpanUnit unit{TimelineSpanUnit::Days};
  TimelineSpanMode mode{TimelineSpanMode::Complete};
  TimelineSpanAnchor anchor{TimelineSpanAnchor::DayOfMonth};
  bool signed_result{true};
  BusinessCalendar calendar{};
  TimelineSpanClosed closed{TimelineSpanClosed::Right};
};

// Lightweight Timeline handle base (similar to CodeM::SeriesBase):
// - cheap copy (Series is ref-counted)
// - shared core invariants for all Timeline-like types
class CODEH_API TimelineBase {
public:
  struct HostArray {
    std::vector<std::int64_t> data;
    std::vector<std::int64_t> sizes;
    std::string tz{"UTC"};
    bool contiguous{true};

    std::span<const std::int64_t> shape() const noexcept { return sizes; }
    std::int64_t dim() const noexcept { return static_cast<std::int64_t>(sizes.size()); }
    std::int64_t numel() const noexcept { return static_cast<std::int64_t>(data.size()); }
  };

  class HostAny final {
  public:
    enum class Kind : std::uint8_t { Scalar = 0, Vector = 1, Array = 2 };

    HostAny() = default;

    static HostAny from_scalar(std::int64_t value, std::string tz) {
      HostAny out;
      out.kind_ = Kind::Scalar;
      out.tz_ = std::move(tz);
      out.payload_ = value;
      return out;
    }

    static HostAny from_vector(std::vector<std::int64_t> values, std::string tz) {
      HostAny out;
      out.kind_ = Kind::Vector;
      out.tz_ = std::move(tz);
      out.payload_ = std::move(values);
      return out;
    }

    static HostAny from_array(HostArray values) {
      HostAny out;
      out.kind_ = Kind::Array;
      out.tz_ = values.tz;
      out.payload_ = std::move(values);
      return out;
    }

    Kind kind() const noexcept { return kind_; }
    const std::string& tz() const noexcept { return tz_; }
    bool is_scalar() const noexcept { return kind_ == Kind::Scalar; }
    bool is_vector() const noexcept { return kind_ == Kind::Vector; }
    bool is_array() const noexcept { return kind_ == Kind::Array; }

    std::int64_t scalar() const {
      CODEM_CHECK(is_scalar(), "Timeline::host(): value is not scalar");
      return std::get<std::int64_t>(payload_);
    }

    const std::vector<std::int64_t>& vector() const {
      CODEM_CHECK(is_vector(), "Timeline::host(): value is not a 1-D vector");
      return std::get<std::vector<std::int64_t>>(payload_);
    }

    const HostArray& array() const {
      CODEM_CHECK(is_array(), "Timeline::host(): value is not N-D array");
      return std::get<HostArray>(payload_);
    }

    friend std::ostream& operator<<(std::ostream& os, const HostAny& value) {
      switch (value.kind_) {
        case Kind::Scalar:
          os << value.scalar();
          break;
        case Kind::Vector:
          os << "[";
          for (std::size_t i = 0; i < value.vector().size(); ++i) {
            if (i) os << ", ";
            os << value.vector()[i];
          }
          os << "]";
          break;
        case Kind::Array:
          os << "Timeline::HostArray(shape=[";
          for (std::size_t i = 0; i < value.array().sizes.size(); ++i) {
            if (i) os << ", ";
            os << value.array().sizes[i];
          }
          os << "], values=" << value.array().data.size() << ", tz=" << value.tz_ << ")";
          break;
      }
      return os;
    }

  private:
    Kind kind_{Kind::Scalar};
    std::string tz_{"UTC"};
    std::variant<std::int64_t, std::vector<std::int64_t>, HostArray> payload_{std::int64_t{0}};
  };

  TimelineBase() = delete;
  TimelineBase(const TimelineBase&) = default;
  TimelineBase(TimelineBase&&) noexcept = default;
  TimelineBase& operator=(const TimelineBase&) = default;
  TimelineBase& operator=(TimelineBase&&) noexcept = default;
  ~TimelineBase() = default;

  bool defined() const noexcept { return ticks_.defined(); }
  pluss::msharp::RangeView::Int sizes() const { return ticks_.sizes(); }
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
  void copy_ticks(OutIt out) const {
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

  std::vector<std::int64_t> ticks() const {
    CODEM_CHECK(defined(), "Timeline: undefined");
    std::vector<std::int64_t> out;
    out.reserve(static_cast<std::size_t>(numel()));
    copy_ticks(std::back_inserter(out));
    return out;
  }

  HostAny host() const {
    CODEM_CHECK(defined(), "Timeline::host(): undefined");
    auto values = ticks();
    if (numel() == 1) {
      CODEM_CHECK(!values.empty(), "Timeline::host(): scalar payload is empty");
      return HostAny::from_scalar(values.front(), tz_name_);
    }
    if (dim() <= 1) {
      return HostAny::from_vector(std::move(values), tz_name_);
    }
    HostArray array;
    array.data = std::move(values);
    array.sizes = sizes().vec();
    array.tz = tz_name_;
    array.contiguous = true;
    return HostAny::from_array(std::move(array));
  }

  // -----------------------------------------------------------------------
  // Additional Timeline operations inherited by Timeline.
  // Definitions are in Timeline.cpp to keep this header lightweight and to
  // avoid include cycles with Timeline/Duration internals.
  // -----------------------------------------------------------------------

  // Scalar field extraction helpers.
  int year_scalar() const;
  int month_scalar() const;
  int day_scalar() const;
  int hour_scalar() const;
  int minute_scalar() const;
  int second_scalar() const;
  int weekday_scalar() const; // 0=Monday..6=Sunday
  int yearday_scalar() const;
  int quarter_scalar() const;
  int week_scalar() const;

  bool leap_scalar() const;
  bool month_start_scalar() const;
  bool month_end_scalar() const;
  bool quarter_start_scalar() const;
  bool quarter_end_scalar() const;
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
  std::string to_string() const;

  pluss::codem::Series to_Series() const;

  Timeline add_years(int years) const;
  Timeline add_months(int months) const;
  Timeline add_days(int days) const;
  Timeline add_hours(int hours) const;
  Timeline add_minutes(int minutes) const;
  Timeline add_seconds(int seconds) const;

  std::int64_t years_between(const Timeline& other) const;
  std::int64_t months_between(const Timeline& other) const;
  std::int64_t days_between(const Timeline& other) const;
  std::int64_t hours_between(const Timeline& other) const;
  std::int64_t minutes_between(const Timeline& other) const;
  std::int64_t seconds_between(const Timeline& other) const;

  Timeline concat(const Timeline& other) const;
  Timeline concat(const std::vector<Timeline>& others) const;
  Timeline repeat(std::int64_t times) const;
  Timeline empty_like() const;
  Timeline fill_like(int year, int month, int day) const;
  Timeline fill_like(const std::string& date_str) const;

  pluss::codem::Series operator==(const Timeline& other) const;
  pluss::codem::Series operator!=(const Timeline& other) const;
  pluss::codem::Series operator<(const Timeline& other) const;
  pluss::codem::Series operator<=(const Timeline& other) const;
  pluss::codem::Series operator>(const Timeline& other) const;
  pluss::codem::Series operator>=(const Timeline& other) const;

  Timeline shift(int periods, std::string_view freq) const;
  Timeline period_start(std::string_view unit) const;
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
  pluss::codem::Series epoch_days() const;
  pluss::codem::Series span(const Timeline& other,
                            const TimelineSpanOptions& options = {}) const;

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
  static std::vector<Timeline> date_range(const Timeline& start,
                                          const Timeline& end,
                                          std::string_view freq = "day",
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
