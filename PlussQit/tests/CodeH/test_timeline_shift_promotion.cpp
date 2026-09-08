#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

TEST_CASE("Timeline shift dispatches only canonical temporal units",
          "[timeline-shift-promotion]") {
  const auto leap = pluss::timeline::make("2024-02-29T08:15:30Z");
  REQUIRE(leap.shift(1, "year").year_scalar() == 2025);
  REQUIRE(leap.shift(1, "year").day_scalar() == 28);
  REQUIRE(leap.shift(1, "month").month_scalar() == 3);
  REQUIRE(leap.shift(1, "day").day_scalar() == 1);
  REQUIRE(leap.shift(1, "hour").hour_scalar() == 9);
  REQUIRE(leap.shift(1, "minute").minute_scalar() == 16);
  REQUIRE(leap.shift(1, "second").second_scalar() == 31);
  REQUIRE(leap.shift(-1, "DAY").day_scalar() == 28);
  REQUIRE(leap.shift(0, "month").ticks() ==
          leap.ticks());

  REQUIRE_THROWS(leap.shift(1, "y"));
  REQUIRE_THROWS(leap.shift(1, "years"));
  REQUIRE_THROWS(leap.shift(1, "min"));
  REQUIRE_THROWS(leap.shift(1, " day "));
}

TEST_CASE("Timeline shift preserves shape timezone and absence",
          "[timeline-shift-promotion]") {
  const auto matrix = pluss::timeline::make({
      {"2024-01-01T00:00:00Z", ""},
      {"2024-01-03T00:00:00Z", "2024-01-04T00:00:00Z"},
  }).tz_convert("America/Edmonton");
  const auto shifted = matrix.shift(2, "hour");
  REQUIRE(shifted.sizes().vec() == std::vector<std::int64_t>{2, 2});
  REQUIRE(shifted.tz() == "America/Edmonton");
  REQUIRE(shifted.isnat().to_vec<bool>() ==
          std::vector<bool>{false, true, false, false});
}

TEST_CASE("Timeline shift distinguishes civil days from elapsed hours",
          "[timeline-shift-promotion]") {
  const auto before = pluss::timeline::make("2024-03-09T10:30:00Z")
                          .tz_convert("America/Edmonton");
  REQUIRE(before.shift(1, "day").hour_scalar() == 3);
  REQUIRE(before.shift(24, "hour").hour_scalar() == 4);
  REQUIRE(before.span(before.shift(1, "day"),
                      pluss::TimelineSpanOptions{
                          .unit = pluss::TimelineSpanUnit::Hours})
              .item<std::int64_t>() == 23);
}

TEST_CASE("Timeline to_timeindex validates axis invariants",
          "[timeline-shift-promotion]") {
  const auto sorted = pluss::timeline::make({
      "2024-01-01T00:00:00Z", "2024-01-01T00:00:00Z",
      "2024-01-03T00:00:00Z"});
  const auto index = sorted.to_timeindex();
  REQUIRE(index.size() == 3);
  REQUIRE(index.is_monotonic_increasing());
  REQUIRE_FALSE(index.is_unique());

  const auto empty = pluss::timeline::epoch({0}).to_timeindex();
  REQUIRE(empty.size() == 0);
  REQUIRE(empty.is_monotonic_increasing());

  const auto unsorted = pluss::timeline::make({
      "2024-01-03T00:00:00Z", "2024-01-01T00:00:00Z"});
  REQUIRE_THROWS(unsorted.to_timeindex());
  REQUIRE_FALSE(unsorted.to_timeindex(false).is_monotonic_increasing());

  REQUIRE_THROWS(pluss::timeline::make({"2024-01-01", ""})
                     .to_timeindex(false));
  REQUIRE_THROWS(pluss::timeline::date("2024-01-01").to_timeindex());
  REQUIRE_THROWS(pluss::timeline::date(
                     std::vector<std::string>{"2024-01-01"}, {1, 1})
                     .to_timeindex());
}

TEST_CASE("Timeline to_timeindex materializes strided axes safely",
          "[timeline-shift-promotion]") {
  std::array<std::int64_t, 5> backing{
      1'704'067'200'000'000'000LL, 99,
      1'704'153'600'000'000'000LL, 88,
      1'704'240'000'000'000'000LL};
  const std::array<std::int64_t, 1> shape{3};
  const std::array<std::int64_t, 1> strides{2};
  const auto source = pluss::Timeline::from_buffer(
      backing.data(), shape, strides, {}, "America/Edmonton");
  const auto index = source.to_timeindex();
  REQUIRE(index.host() == std::vector<std::int64_t>{
      backing[0], backing[2], backing[4]});
  REQUIRE(index.timeline().tz() == "America/Edmonton");
  REQUIRE(index.searchsorted(pluss::timeline::date("2024-01-02")) == 1);
}
