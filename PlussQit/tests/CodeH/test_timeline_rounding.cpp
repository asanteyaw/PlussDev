#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <vector>

TEST_CASE("Timeline rounding handles negative epochs and half-even ties",
          "[timeline-rounding]") {
  const auto values = pluss::timeline::make({
      "1969-12-31T23:29:30.123456789Z", "1969-12-31T23:30:00Z",
      "1970-01-01T00:30:00Z", "1970-01-01T01:30:00Z", "",
  });
  REQUIRE(values.floor("H").ticks() == pluss::timeline::make({
      "1969-12-31T23:00:00Z", "1969-12-31T23:00:00Z",
      "1970-01-01T00:00:00Z", "1970-01-01T01:00:00Z", "",
  }).ticks());
  REQUIRE(values.ceil("H").ticks() == pluss::timeline::make({
      "1970-01-01T00:00:00Z", "1970-01-01T00:00:00Z",
      "1970-01-01T01:00:00Z", "1970-01-01T02:00:00Z", "",
  }).ticks());
  REQUIRE(values.round("H").ticks() == pluss::timeline::make({
      "1969-12-31T23:00:00Z", "1970-01-01T00:00:00Z",
      "1970-01-01T00:00:00Z", "1970-01-01T02:00:00Z", "",
  }).ticks());
}

TEST_CASE("Timeline rounding resolves DST boundaries in local time",
          "[timeline-rounding]") {
  const auto noon = pluss::timeline::make(
      "2024-03-10T18:00:00Z").tz_convert("America/Edmonton");
  REQUIRE(noon.date().isoformat() == "2024-03-10T00:00:00.000000000-07:00");

  const auto folds = pluss::timeline::make({
      "2024-11-03T07:45:00Z", "2024-11-03T08:45:00Z",
  }).tz_convert("America/Edmonton");
  REQUIRE(folds.floor("H").ticks() == pluss::timeline::make({
      "2024-11-03T07:00:00Z", "2024-11-03T08:00:00Z",
  }).ticks());

  const auto spring = pluss::timeline::make(
      "2024-03-10T08:30:00Z").tz_convert("America/Edmonton");
  REQUIRE_THROWS(spring.ceil("H"));
}

TEST_CASE("Timeline date and time preserve shape absence and local fields",
          "[timeline-rounding]") {
  const auto clock = pluss::timeline::make(
      "2024-07-15T18:34:56.123456789Z").tz_convert("America/Edmonton");
  REQUIRE(clock.time().isoformat() == "1970-01-01T12:34:56.123456789-07:00");

  const auto clocks = pluss::timeline::make({
      "2024-07-15T18:34:56.123456789Z", "",
  }).tz_convert("America/Edmonton");
  REQUIRE(clocks.time().isnat().host<bool>() == std::vector<bool>{false, true});
  REQUIRE(clocks.date().isnat().host<bool>() == std::vector<bool>{false, true});
  REQUIRE_THROWS(clocks.floor("week"));
}

TEST_CASE("timeline date and time factories project existing Timelines",
          "[timeline-rounding][timeline-factory-components]") {
  const auto schedule = pluss::timeline::make({
      {"2024-01-01T00:00:00.000000001Z", "2024-04-01T06:30:00Z"},
      {"2024-07-01T12:45:30.250Z", ""},
  }).tz_convert("America/Edmonton");

  const auto dates = pluss::timeline::date(schedule);
  const auto times = pluss::timeline::time(schedule);

  REQUIRE(dates.ticks() == schedule.date().ticks());
  REQUIRE(times.ticks() == schedule.time().ticks());
  REQUIRE(dates.sizes().vec() == std::vector<std::int64_t>{2, 2});
  REQUIRE(times.sizes().vec() == std::vector<std::int64_t>{2, 2});
  REQUIRE(dates.tz() == schedule.tz());
  REQUIRE(times.tz() == schedule.tz());
  REQUIRE(dates.isnat().host<bool>() ==
          std::vector<bool>{false, false, false, true});
  REQUIRE(times.isnat().host<bool>() ==
          std::vector<bool>{false, false, false, true});

  REQUIRE(pluss::timeline::date(dates).ticks() == dates.ticks());
  REQUIRE(pluss::timeline::time(times).ticks() == times.ticks());
}
