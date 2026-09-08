#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace {
pluss::TimelineSpanOptions with_unit(pluss::TimelineSpanUnit unit) {
  auto options = pluss::TimelineSpanOptions{};
  options.unit = unit;
  return options;
}
} // namespace

TEST_CASE("Timeline span computes widened fixed-unit differences",
          "[timeline-span]") {
  const auto start = pluss::timeline::make("2024-01-01T00:00:00.123456789Z");
  const auto end = pluss::timeline::make("2024-01-02T01:02:03.123456999Z");
  REQUIRE(start.span(end).item<std::int64_t>() == 1);
  REQUIRE(start.span(end, with_unit(pluss::TimelineSpanUnit::Nanos))
              .item<std::int64_t>() == 90'123'000'000'210LL);
  REQUIRE(start.span(end, with_unit(pluss::TimelineSpanUnit::Micros))
              .item<std::int64_t>() == 90'123'000'000LL);
  REQUIRE(start.span(end, with_unit(pluss::TimelineSpanUnit::Millis))
              .item<std::int64_t>() == 90'123'000LL);
  REQUIRE(start.span(end, with_unit(pluss::TimelineSpanUnit::Seconds))
              .item<std::int64_t>() == 90'123LL);
  REQUIRE(start.span(end, with_unit(pluss::TimelineSpanUnit::Minutes))
              .item<std::int64_t>() == 1'502LL);
  REQUIRE(start.span(end, with_unit(pluss::TimelineSpanUnit::Hours))
              .item<std::int64_t>() == 25LL);
  REQUIRE(start.span(end, with_unit(pluss::TimelineSpanUnit::Weeks))
              .item<std::int64_t>() == 0);

  const auto near_min = pluss::timeline::from_epoch(
      std::numeric_limits<std::int64_t>::min() + 1);
  const auto near_max = pluss::timeline::from_epoch(
      std::numeric_limits<std::int64_t>::max());
  REQUIRE(near_min.span(near_max, with_unit(pluss::TimelineSpanUnit::Seconds))
              .item<std::int64_t>() == 18'446'744'073LL);
  REQUIRE_THROWS(near_min.span(
      near_max, with_unit(pluss::TimelineSpanUnit::Nanos)));
}

TEST_CASE("Timeline span distinguishes elapsed days from civil days",
          "[timeline-span]") {
  const auto spring = pluss::timeline::make("2024-03-09T10:30:00Z")
                          .tz_convert("America/Edmonton");
  const auto next_day = spring.add_days(1);
  REQUIRE(spring.span(next_day).item<std::int64_t>() == 0);
  REQUIRE(spring.span(next_day, with_unit(pluss::TimelineSpanUnit::Hours))
              .item<std::int64_t>() == 23);
}

TEST_CASE("Timeline span complete calendar units include local time of day",
          "[timeline-span]") {
  const auto jan31_noon = pluss::timeline::make("2024-01-31T12:00:00Z");
  const auto feb29_early =
      pluss::timeline::make("2024-02-29T11:59:59.999999999Z");
  const auto feb29_noon = pluss::timeline::make("2024-02-29T12:00:00Z");
  auto months = with_unit(pluss::TimelineSpanUnit::Months);
  REQUIRE(jan31_noon.span(feb29_noon, months).item<std::int64_t>() == 0);
  months.anchor = pluss::TimelineSpanAnchor::MonthEnd;
  REQUIRE(jan31_noon.span(feb29_early, months).item<std::int64_t>() == 0);
  REQUIRE(jan31_noon.span(feb29_noon, months).item<std::int64_t>() == 1);
  months.mode = pluss::TimelineSpanMode::Boundary;
  months.anchor = pluss::TimelineSpanAnchor::DayOfMonth;
  REQUIRE(jan31_noon.span(feb29_early, months).item<std::int64_t>() == 1);

  const auto leap = pluss::timeline::make("2024-02-29T12:00:00Z");
  const auto next_feb_end = pluss::timeline::make("2025-02-28T12:00:00Z");
  auto years = with_unit(pluss::TimelineSpanUnit::Years);
  REQUIRE(leap.span(next_feb_end, years).item<std::int64_t>() == 0);
  years.anchor = pluss::TimelineSpanAnchor::MonthEnd;
  REQUIRE(leap.span(next_feb_end, years).item<std::int64_t>() == 1);
}

TEST_CASE("Timeline span handles sign broadcasting strides absence and empty shapes",
          "[timeline-span]") {
  const auto scalar = pluss::timeline::date("2024-01-01");
  const auto vector = pluss::timeline::make(
      {"2024-01-02T00:00:00Z", "2024-01-03T12:00:00Z"});
  REQUIRE(scalar.span(vector).to_vec<std::int64_t>() ==
          std::vector<std::int64_t>{1, 2});
  REQUIRE(vector.span(scalar).to_vec<std::int64_t>() ==
          std::vector<std::int64_t>{-1, -2});
  auto absolute = pluss::TimelineSpanOptions{};
  absolute.signed_result = false;
  REQUIRE(vector.span(scalar, absolute).to_vec<std::int64_t>() ==
          std::vector<std::int64_t>{1, 2});

  const std::vector<std::string> column_values{
      "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z"};
  const std::vector<std::string> row_values{
      "2024-01-03T00:00:00Z", "2024-01-05T00:00:00Z"};
  const auto column = pluss::timeline::make(column_values, {2, 1});
  const auto row = pluss::timeline::make(row_values, {1, 2});
  const auto matrix = column.span(row);
  REQUIRE(matrix.sizes().vec() == std::vector<std::int64_t>{2, 2});
  REQUIRE(matrix.to_vec<std::int64_t>() ==
          std::vector<std::int64_t>{2, 4, 1, 3});

  std::array<std::int64_t, 4> backing{
      1'704'067'200'000'000'000LL, 111LL,
      1'704'153'600'000'000'000LL, 222LL};
  const std::array<std::int64_t, 1> shape{2};
  const std::array<std::int64_t, 1> strides{2};
  const auto strided = pluss::Timeline::from_buffer(
      backing.data(), shape, strides, {}, "UTC");
  REQUIRE(scalar.span(strided).to_vec<std::int64_t>() ==
          std::vector<std::int64_t>{0, 1});

  const auto absent = pluss::timeline::make({"2024-01-01", ""});
  REQUIRE(absent.span(pluss::timeline::date("2024-01-03"))
              .to_vec<std::int64_t>() ==
          std::vector<std::int64_t>{2, pluss::Timeline::kNaT});
  const auto empty = pluss::timeline::epoch({0, 2}).span(
      pluss::timeline::from_epoch(0LL));
  REQUIRE(empty.sizes().vec() == std::vector<std::int64_t>{0, 2});
}

TEST_CASE("Timeline span applies business calendars and closure",
          "[timeline-span]") {
  const auto friday = pluss::timeline::make("2024-07-05T15:00:00Z");
  const auto monday = pluss::timeline::make("2024-07-08T09:00:00Z");
  auto business = with_unit(pluss::TimelineSpanUnit::BusinessDays);
  REQUIRE(friday.span(monday, business).item<std::int64_t>() == 1);
  business.closed = pluss::TimelineSpanClosed::Both;
  REQUIRE(friday.span(monday, business).item<std::int64_t>() == 2);
  business.closed = pluss::TimelineSpanClosed::Left;
  REQUIRE(friday.span(monday, business).item<std::int64_t>() == 1);
  business.closed = pluss::TimelineSpanClosed::Neither;
  REQUIRE(friday.span(monday, business).item<std::int64_t>() == 0);
  business.closed = pluss::TimelineSpanClosed::Right;
  business.calendar.holidays =
      pluss::timeline::date("2024-07-08").epoch_days().to_vec<std::int64_t>();
  REQUIRE(friday.span(monday, business).item<std::int64_t>() == 0);
  business.calendar = pluss::BusinessCalendar::monfri();
  business.calendar.weekmask = 0b1000000;
  REQUIRE(friday.span(monday, business).item<std::int64_t>() == 1);

  business.calendar = pluss::BusinessCalendar::canada_federal(2024, 2024);
  const auto before = pluss::timeline::make("2024-06-28T12:00:00Z");
  const auto after = pluss::timeline::make("2024-07-02T12:00:00Z");
  REQUIRE(before.span(after, business).item<std::int64_t>() == 1);
  business.calendar = pluss::BusinessCalendar::monfri();
  REQUIRE(before.span(after, business).item<std::int64_t>() == 2);
}

TEST_CASE("Timeline span validates calendar options", "[timeline-span]") {
  const auto utc = pluss::timeline::date("2024-01-01");
  const auto local = utc.tz_convert("America/Edmonton");
  REQUIRE_THROWS(utc.span(local, with_unit(pluss::TimelineSpanUnit::Months)));
  REQUIRE_NOTHROW(utc.span(local, with_unit(pluss::TimelineSpanUnit::Seconds)));

  auto business = with_unit(pluss::TimelineSpanUnit::BusinessDays);
  business.calendar.weekmask = 0;
  REQUIRE_THROWS(utc.span(pluss::timeline::date("2024-01-02"), business));
  business.calendar.weekmask = 0b10000000;
  REQUIRE_THROWS(utc.span(pluss::timeline::date("2024-01-02"), business));
  business.calendar = pluss::BusinessCalendar::nyse(2024, 2024);
  REQUIRE_THROWS(utc.span(pluss::timeline::date("2024-01-02"), business));

  const auto matrix = pluss::timeline::make(
      {{"2024-01-01", "2024-01-02"}, {"2024-01-03", "2024-01-04"}});
  const auto vector = pluss::timeline::make(
      {"2024-01-01", "2024-01-02", "2024-01-03"});
  REQUIRE_THROWS(matrix.span(vector));
}
