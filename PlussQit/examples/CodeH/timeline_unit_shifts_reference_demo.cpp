#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <string_view>

namespace {
void show(std::string_view label, const pluss::Timeline& value) {
  std::cout << "\n=== " << label << " ===\n"
            << pluss::note::print(
                   pluss::timeline::format(value, "%FT%T.%f%z"))
            << '\n';
}

void show(std::string_view label, const pluss::Series& value) {
  std::cout << "\n=== " << label << " ===\n"
            << pluss::series::print(value) << '\n';
}

template <class Fn>
void show_rejection(std::string_view label, Fn&& fn) {
  try {
    fn();
  } catch (const pluss::Error&) {
    std::cout << "\n=== " << label << " ===\nrejected\n";
  }
}
} // namespace

int main() {
  const auto calendar = pluss::timeline::make({
      {"2024-02-29T08:15:30.123456789Z", "2023-01-31T23:00:00Z"},
      {"2024-03-31T12:00:00Z", "2024-12-31T23:59:59.999999999Z"},
  });
  show("calendar_source", calendar);
  show("add_years_positive", calendar.add_years(1));
  show("add_years_negative", calendar.add_years(-1));
  const auto leap_scalar = pluss::timeline::make(
      "2024-02-29T08:15:30.123456789Z");
  show("add_years_zero", leap_scalar.add_years(0));
  show("add_years_source_unchanged", leap_scalar);
  show("add_months_positive", calendar.add_months(1));
  show("add_months_negative", calendar.add_months(-1));
  show("add_months_year_boundary",
       pluss::timeline::make("2024-11-30T06:00:00Z").add_months(15));
  show("add_months_local_offset",
       pluss::timeline::make("2024-02-10T10:30:00Z")
           .tz_convert("America/Edmonton")
           .add_months(1));
  show("add_months_absent",
       pluss::timeline::from_epoch(
           {1'704'067'200'123'456'789LL, pluss::Timeline::kNaT}, "UTC")
           .add_months(1));

  const auto spring = pluss::timeline::make("2024-03-09T10:30:00Z")
                          .tz_convert("America/Edmonton");
  show("spring_source", spring);
  show("add_calendar_day_across_spring", spring.add_days(1));
  show("add_elapsed_hours_across_spring", spring.add_hours(24));
  show("add_two_elapsed_hours_across_spring",
       pluss::timeline::make("2024-03-10T08:30:00Z")
           .tz_convert("America/Edmonton")
           .add_hours(2));

  const auto precise = pluss::timeline::make(
      "2024-12-31T23:59:30.123456789Z");
  show("add_days_positive", precise.add_days(1));
  show("add_days_negative", precise.add_days(-1));
  show("add_days_zero", precise.add_days(0));
  show("add_hours_negative",
       pluss::timeline::make("2025-01-01T01:15:00.123456789Z")
           .add_hours(-3));
  show("add_minutes_crosses_year", precise.add_minutes(2));
  show("add_minutes_negative", precise.add_minutes(-2));
  show("add_minutes_zero", precise.add_minutes(0));
  show("add_minutes_matrix",
       pluss::timeline::make({
           {"2024-01-01T09:00:00Z", "2024-01-01T09:30:15.000000004Z"},
           {"", "2024-01-01T23:59:45Z"},
       }).add_minutes(1));
  show("add_seconds_crosses_midnight", precise.add_seconds(45));
  show("subtract_seconds", precise.add_seconds(-90));
  show("zero_shift", precise.add_seconds(0));
  show("source_is_unchanged", precise);

  const auto absent = pluss::timeline::from_epoch(
      {1'704'067'200'123'456'789LL, pluss::Timeline::kNaT}, "UTC");
  show("absent_propagates", absent.add_days(3));
  show("add_seconds_absent", absent.add_seconds(1));
  show("empty_shape", pluss::timeline::epoch({0, 2}).add_days(1));

  std::array<std::int64_t, 4> backing{
      1'704'067'200'123'456'789LL, 111LL,
      1'709'164'800'000'000'004LL, 222LL};
  const std::array<std::int64_t, 1> shape{2};
  const std::array<std::int64_t, 1> strides{2};
  const auto strided = pluss::Timeline::from_buffer(
      backing.data(), shape, strides, {}, "UTC");
  show("strided_source", strided);
  show("strided_add_years", strided.add_years(1));
  show("strided_add_hours", strided.add_hours(1));

  show("elapsed_difference_nanos",
       spring.add_days(1).epoch_nanos() - spring.epoch_nanos());

  show_rejection("calendar_day_into_dst_gap", [] {
    const auto before = pluss::timeline::make("2024-03-09T09:30:00Z")
                            .tz_convert("America/Edmonton");
    (void)before.add_days(1);
  });
  show_rejection("calendar_month_into_dst_gap", [] {
    const auto before = pluss::timeline::make("2024-02-10T09:30:00Z")
                            .tz_convert("America/Edmonton");
    (void)before.add_months(1);
  });
  show_rejection("year_out_of_calendar_range", [] {
    (void)pluss::timeline::make("9999-12-31T00:00:00Z").add_years(1);
  });
  show_rejection("elapsed_tick_overflow", [] {
    (void)pluss::timeline::from_epoch(
        std::int64_t{9'223'372'036'854'775'000LL}).add_seconds(1);
  });
  show_rejection("unit_scale_overflow", [] {
    (void)pluss::timeline::make("1970-01-01T00:00:00Z")
        .add_hours(2'147'483'647);
  });
  show_rejection("minute_scale_overflow", [] {
    (void)pluss::timeline::make("1970-01-01T00:00:00Z")
        .add_minutes(2'147'483'647);
  });
}
