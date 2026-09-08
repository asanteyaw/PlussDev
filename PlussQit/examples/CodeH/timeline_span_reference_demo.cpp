#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace {
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

pluss::TimelineSpanOptions with_unit(pluss::TimelineSpanUnit unit) {
  auto options = pluss::TimelineSpanOptions{};
  options.unit = unit;
  return options;
}
} // namespace

int main() {
  const auto start = pluss::timeline::make("2024-01-01T00:00:00.123456789Z");
  const auto end = pluss::timeline::make("2024-01-02T01:02:03.123456999Z");
  show("default_elapsed_days", start.span(end));
  show("elapsed_nanos", start.span(end, with_unit(pluss::TimelineSpanUnit::Nanos)));
  show("elapsed_micros", start.span(end, with_unit(pluss::TimelineSpanUnit::Micros)));
  show("elapsed_millis", start.span(end, with_unit(pluss::TimelineSpanUnit::Millis)));
  show("elapsed_seconds", start.span(end, with_unit(pluss::TimelineSpanUnit::Seconds)));
  show("elapsed_minutes", start.span(end, with_unit(pluss::TimelineSpanUnit::Minutes)));
  show("elapsed_hours", start.span(end, with_unit(pluss::TimelineSpanUnit::Hours)));
  show("elapsed_weeks", start.span(end, with_unit(pluss::TimelineSpanUnit::Weeks)));

  const auto spring = pluss::timeline::make("2024-03-09T10:30:00Z")
                          .tz_convert("America/Edmonton");
  const auto next_civil_day = spring.add_days(1);
  show("dst_elapsed_hours",
       spring.span(next_civil_day, with_unit(pluss::TimelineSpanUnit::Hours)));
  show("dst_elapsed_days", spring.span(next_civil_day));

  const auto scalar = pluss::timeline::make("2024-01-01T00:00:00Z");
  const auto vector = pluss::timeline::make(
      {"2024-01-02T00:00:00Z", "2024-01-03T12:00:00Z"});
  show("broadcast_scalar_vector", scalar.span(vector));
  const std::vector<std::string> column_values{
      "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z"};
  const std::vector<std::string> row_values{
      "2024-01-03T00:00:00Z", "2024-01-05T00:00:00Z"};
  const auto column = pluss::timeline::make(column_values, {2, 1});
  const auto row = pluss::timeline::make(row_values, {1, 2});
  show("broadcast_matrix", column.span(row));

  const auto reverse_options = with_unit(pluss::TimelineSpanUnit::Seconds);
  show("signed_reverse", end.span(start, reverse_options));
  auto absolute_options = reverse_options;
  absolute_options.signed_result = false;
  show("absolute_reverse", end.span(start, absolute_options));

  const auto jan31_noon = pluss::timeline::make("2024-01-31T12:00:00Z");
  const auto feb29_early =
      pluss::timeline::make("2024-02-29T11:59:59.999999999Z");
  const auto feb29_noon = pluss::timeline::make("2024-02-29T12:00:00Z");
  auto months = with_unit(pluss::TimelineSpanUnit::Months);
  show("months_complete_day_of_month", jan31_noon.span(feb29_noon, months));
  months.anchor = pluss::TimelineSpanAnchor::MonthEnd;
  show("months_month_end_one_ns_short", jan31_noon.span(feb29_early, months));
  show("months_month_end_complete", jan31_noon.span(feb29_noon, months));
  months.mode = pluss::TimelineSpanMode::Boundary;
  months.anchor = pluss::TimelineSpanAnchor::DayOfMonth;
  show("months_boundary", jan31_noon.span(feb29_early, months));

  const auto leap = pluss::timeline::make("2024-02-29T12:00:00Z");
  const auto next_feb_end = pluss::timeline::make("2025-02-28T12:00:00Z");
  auto years = with_unit(pluss::TimelineSpanUnit::Years);
  show("years_complete_day_of_month", leap.span(next_feb_end, years));
  years.anchor = pluss::TimelineSpanAnchor::MonthEnd;
  show("years_complete_month_end", leap.span(next_feb_end, years));

  auto business = with_unit(pluss::TimelineSpanUnit::BusinessDays);
  const auto friday = pluss::timeline::make("2024-07-05T15:00:00Z");
  const auto monday = pluss::timeline::make("2024-07-08T09:00:00Z");
  show("business_right_closed", friday.span(monday, business));
  business.closed = pluss::TimelineSpanClosed::Both;
  show("business_both_closed", friday.span(monday, business));
  business.closed = pluss::TimelineSpanClosed::Left;
  show("business_left_closed", friday.span(monday, business));
  business.closed = pluss::TimelineSpanClosed::Neither;
  show("business_neither_closed", friday.span(monday, business));
  business.closed = pluss::TimelineSpanClosed::Right;
  business.calendar.holidays =
      pluss::timeline::date("2024-07-08").epoch_days().to_vec<std::int64_t>();
  show("business_custom_holiday", friday.span(monday, business));
  business.calendar = pluss::BusinessCalendar::monfri();
  business.calendar.weekmask = 0b1000000;
  show("business_custom_weekmask", friday.span(monday, business));

  business.calendar = pluss::BusinessCalendar::canada_federal(2024, 2024);
  const auto before_canada_day = pluss::timeline::make("2024-06-28T12:00:00Z");
  const auto after_canada_day = pluss::timeline::make("2024-07-02T12:00:00Z");
  show("business_canada_federal",
       before_canada_day.span(after_canada_day, business));
  business.calendar = pluss::BusinessCalendar::monfri();
  show("business_plain_weekdays",
       before_canada_day.span(after_canada_day, business));

  const auto absent = pluss::timeline::make({"2024-01-01", ""});
  show("absent_propagates", absent.span(pluss::timeline::date("2024-01-03")));
  show("empty_broadcast",
       pluss::timeline::epoch({0, 2}).span(pluss::timeline::from_epoch(0LL)));

  std::array<std::int64_t, 4> backing{
      1'704'067'200'000'000'000LL, 111LL,
      1'704'153'600'000'000'000LL, 222LL};
  const std::array<std::int64_t, 1> shape{2};
  const std::array<std::int64_t, 1> strides{2};
  const auto strided = pluss::Timeline::from_buffer(
      backing.data(), shape, strides, {}, "UTC");
  show("strided_span", scalar.span(strided));

  const auto near_min = pluss::timeline::from_epoch(
      std::numeric_limits<std::int64_t>::min() + 1);
  const auto near_max = pluss::timeline::from_epoch(
      std::numeric_limits<std::int64_t>::max());
  show("wide_seconds_still_fit",
       near_min.span(near_max, with_unit(pluss::TimelineSpanUnit::Seconds)));
  show_rejection("wide_nanos_rejected", [&] {
    (void)near_min.span(near_max, with_unit(pluss::TimelineSpanUnit::Nanos));
  });
  show_rejection("calendar_timezone_mismatch", [] {
    const auto utc = pluss::timeline::make("2024-01-01T00:00:00Z");
    const auto local = utc.tz_convert("America/Edmonton");
    (void)utc.span(local, with_unit(pluss::TimelineSpanUnit::Months));
  });
  show_rejection("invalid_business_weekmask", [] {
    auto options = with_unit(pluss::TimelineSpanUnit::BusinessDays);
    options.calendar.weekmask = 0;
    (void)pluss::timeline::date("2024-01-01").span(
        pluss::timeline::date("2024-01-02"), options);
  });
  show_rejection("incompatible_shapes", [] {
    const auto lhs = pluss::timeline::make(
        {{"2024-01-01", "2024-01-02"}, {"2024-01-03", "2024-01-04"}});
    const auto rhs = pluss::timeline::make(
        {"2024-01-01", "2024-01-02", "2024-01-03"});
    (void)lhs.span(rhs);
  });
}
