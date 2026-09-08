#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <limits>
#include <string_view>
#include <vector>

namespace {
std::int64_t tick(const pluss::Timeline& value) {
  return value.ticks().at(0);
}
} // namespace

TEST_CASE("Timeline year and month shifts use local calendar arithmetic",
          "[timeline-unit-shifts]") {
  const auto leap = pluss::timeline::make("2024-02-29T08:15:30.123456789Z");
  REQUIRE(pluss::timeline::format(leap.add_years(1), "%FT%T.%f%z").scalar() ==
          "2025-02-28T08:15:30.123456789+00:00");
  REQUIRE(pluss::timeline::format(leap.add_years(-1), "%FT%T.%f%z").scalar() ==
          "2023-02-28T08:15:30.123456789+00:00");

  const auto month_end = pluss::timeline::make({
      "2024-01-31T08:00:00Z", "2024-03-31T08:00:00Z",
      "2024-11-30T08:00:00Z"});
  REQUIRE(pluss::timeline::format(month_end.add_months(1), "%F").vector() ==
          std::vector<std::string_view>{"2024-02-29", "2024-04-30", "2024-12-30"});
  REQUIRE(pluss::timeline::format(month_end.add_months(-1), "%F").vector() ==
          std::vector<std::string_view>{"2023-12-31", "2024-02-29", "2024-10-30"});
  REQUIRE(pluss::timeline::format(month_end.add_months(15), "%F").vector() ==
          std::vector<std::string_view>{"2025-04-30", "2025-06-30", "2026-02-28"});
  REQUIRE(month_end.add_months(0).ticks() ==
          month_end.ticks());
}

TEST_CASE("Timeline day shifts preserve wall time while smaller units preserve elapsed time",
          "[timeline-unit-shifts]") {
  constexpr std::int64_t hour = 3'600'000'000'000LL;
  const auto spring = pluss::timeline::make("2024-03-09T10:30:00Z")
                          .tz_convert("America/Edmonton");
  const auto calendar_day = spring.add_days(1);
  const auto elapsed_day = spring.add_hours(24);

  REQUIRE(pluss::timeline::format(calendar_day, "%FT%T%z").scalar() ==
          "2024-03-10T03:30:00-06:00");
  REQUIRE(pluss::timeline::format(elapsed_day, "%FT%T%z").scalar() ==
          "2024-03-10T04:30:00-06:00");
  REQUIRE(tick(calendar_day) - tick(spring) == 23 * hour);
  REQUIRE(tick(elapsed_day) - tick(spring) == 24 * hour);

  const auto fall = pluss::timeline::make("2024-11-02T09:30:00Z")
                        .tz_convert("America/Edmonton");
  REQUIRE(tick(fall.add_days(1)) - tick(fall) == 25 * hour);

  const auto gap_source = pluss::timeline::make("2024-03-09T09:30:00Z")
                              .tz_convert("America/Edmonton");
  REQUIRE_THROWS(gap_source.add_days(1));
}

TEST_CASE("Timeline elapsed unit shifts retain precision and reject overflow",
          "[timeline-unit-shifts]") {
  const auto value = pluss::timeline::make("2024-12-31T23:59:30.123456789Z");
  REQUIRE(pluss::timeline::format(value.add_minutes(2), "%FT%T.%f%z").scalar() ==
          "2025-01-01T00:01:30.123456789+00:00");
  REQUIRE(pluss::timeline::format(value.add_seconds(45), "%FT%T.%f%z").scalar() ==
          "2025-01-01T00:00:15.123456789+00:00");
  REQUIRE(tick(value.add_hours(-2)) == tick(value) - 7'200'000'000'000LL);
  REQUIRE(tick(value.add_minutes(-2)) == tick(value) - 120'000'000'000LL);
  REQUIRE(tick(value.add_seconds(-2)) == tick(value) - 2'000'000'000LL);
  REQUIRE(value.add_seconds(0).ticks() ==
          value.ticks());

  REQUIRE_THROWS(pluss::timeline::from_epoch(
      std::numeric_limits<std::int64_t>::max() - 500'000'000LL).add_seconds(1));
  REQUIRE_THROWS(value.add_hours(std::numeric_limits<int>::max()));
  REQUIRE_THROWS(value.add_minutes(std::numeric_limits<int>::max()));
}

TEST_CASE("Timeline unit shifts preserve shape timezone absence and logical strides",
          "[timeline-unit-shifts]") {
  const auto matrix = pluss::timeline::make({
      {"2024-01-01T00:00:00Z", "2024-02-29T00:00:00Z"},
      {"", "2024-12-31T23:59:59.000000001Z"},
  }).tz_convert("America/New_York");
  const auto shifted = matrix.add_years(1).add_seconds(1);
  REQUIRE(shifted.sizes().vec() == std::vector<std::int64_t>{2, 2});
  REQUIRE(shifted.tz() == "America/New_York");
  REQUIRE(shifted.isnat().to_vec<bool>() ==
          std::vector<bool>{false, false, true, false});

  std::array<std::int64_t, 4> backing{
      1'704'067'200'123'456'789LL, 111LL,
      1'709'164'800'000'000'004LL, 222LL};
  const std::array<std::int64_t, 1> shape{2};
  const std::array<std::int64_t, 1> strides{2};
  const auto strided = pluss::Timeline::from_buffer(
      backing.data(), shape, strides, {}, "UTC");
  REQUIRE(strided.add_hours(1).ticks() ==
          std::vector<std::int64_t>{1'704'070'800'123'456'789LL,
                                    1'709'168'400'000'000'004LL});
  REQUIRE(strided.add_years(1).year().to_vec<std::int32_t>() ==
          std::vector<std::int32_t>{2025, 2025});

  const auto empty = pluss::timeline::epoch({0, 2}).add_months(1);
  REQUIRE(empty.sizes().vec() == std::vector<std::int64_t>{0, 2});
  REQUIRE(empty.ticks().empty());
}
