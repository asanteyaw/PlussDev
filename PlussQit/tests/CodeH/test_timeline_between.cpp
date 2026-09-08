#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cstdint>

TEST_CASE("Timeline calendar between counts clamped anniversaries",
          "[timeline-between]") {
  const auto month_start = pluss::timeline::make("2023-01-31T10:00:00Z");
  const auto month_anniversary = pluss::timeline::make("2023-02-28T10:00:00Z");
  const auto before_month = pluss::timeline::make("2023-02-28T09:59:59Z");
  REQUIRE(month_start.months_between(month_anniversary) == 1);
  REQUIRE(month_start.months_between(before_month) == 0);
  REQUIRE(month_anniversary.months_between(month_start) == -1);

  const auto leap = pluss::timeline::make("2024-02-29T12:00:00Z");
  const auto year_anniversary = pluss::timeline::make("2025-02-28T12:00:00Z");
  const auto before_year = pluss::timeline::make("2025-02-28T11:59:59Z");
  REQUIRE(leap.years_between(year_anniversary) == 1);
  REQUIRE(leap.years_between(before_year) == 0);
  REQUIRE(year_anniversary.years_between(leap) == -1);
}

TEST_CASE("Timeline elapsed between truncates incomplete signed units",
          "[timeline-between]") {
  const auto start = pluss::timeline::make("2024-01-01T00:00:00Z");
  const auto end = pluss::timeline::make("2024-01-02T02:03:04.500000000Z");
  REQUIRE(start.days_between(end) == 1);
  REQUIRE(start.hours_between(end) == 26);
  REQUIRE(start.minutes_between(end) == 1563);
  REQUIRE(start.seconds_between(end) == 93'784);
  REQUIRE(end.days_between(start) == -1);
  REQUIRE(end.hours_between(start) == -26);
  REQUIRE(end.minutes_between(start) == -1563);
  REQUIRE(end.seconds_between(start) == -93'784);
}

TEST_CASE("Timeline between separates civil days from elapsed days",
          "[timeline-between]") {
  const auto before = pluss::timeline::make("2024-03-09T10:30:00Z")
                          .tz_convert("America/Edmonton");
  const auto next_civil_day = before.add_days(1);
  REQUIRE(before.hours_between(next_civil_day) == 23);
  REQUIRE(before.days_between(next_civil_day) == 0);
  REQUIRE(before.days_between(before.add_hours(24)) == 1);
}

TEST_CASE("Timeline between remains safe across the full tick range",
          "[timeline-between]") {
  const auto early = pluss::timeline::make("1700-01-01T00:00:00Z");
  const auto late = pluss::timeline::make("2200-01-01T00:00:00Z");
  REQUIRE(early.seconds_between(late) == 15'778'454'400LL);
  REQUIRE(late.seconds_between(early) == -15'778'454'400LL);
  REQUIRE(early.seconds_between(late) >
          static_cast<std::int64_t>(2'147'483'647));
}

TEST_CASE("Timeline between validates scalar temporal operands",
          "[timeline-between]") {
  const auto scalar = pluss::timeline::make("2024-01-01T00:00:00Z");
  const auto same_in_zone = scalar.tz_convert("America/Edmonton");
  REQUIRE(scalar.seconds_between(same_in_zone) == 0);
  REQUIRE_THROWS(scalar.months_between(same_in_zone));
  REQUIRE_THROWS(scalar.years_between(same_in_zone));
  REQUIRE_THROWS(scalar.seconds_between(pluss::timeline::make("")));
  REQUIRE_THROWS(scalar.days_between(
      pluss::timeline::make({"2024-01-01", "2024-01-02"})));
}
