#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace {

std::vector<std::string> formatted(const pluss::Timeline& value) {
  const auto text = pluss::timeline::format(value, "%FT%T.%f%z");
  std::vector<std::string> out;
  out.reserve(static_cast<std::size_t>(text.numel()));
  for (std::int64_t i = 0; i < text.numel(); ++i) {
    out.emplace_back(text.flat_at(i));
  }
  return out;
}

} // namespace

TEST_CASE("Timeline period_start projects all canonical units in local time",
          "[timeline-period-start]") {
  const auto value = pluss::timeline::make(
      "2024-07-15T18:34:56.123456789Z").tz_convert("America/Edmonton");

  REQUIRE(formatted(value.period_start("year")) ==
          std::vector<std::string>{"2024-01-01T00:00:00.000000000-07:00"});
  REQUIRE(formatted(value.period_start("month")) ==
          std::vector<std::string>{"2024-07-01T00:00:00.000000000-06:00"});
  REQUIRE(formatted(value.period_start("day")) ==
          std::vector<std::string>{"2024-07-15T00:00:00.000000000-06:00"});
  REQUIRE(formatted(value.period_start("hour")) ==
          std::vector<std::string>{"2024-07-15T12:00:00.000000000-06:00"});
  REQUIRE(formatted(value.period_start("minute")) ==
          std::vector<std::string>{"2024-07-15T12:34:00.000000000-06:00"});
  REQUIRE(formatted(value.period_start("second")) ==
          std::vector<std::string>{"2024-07-15T12:34:56.000000000-06:00"});
}

TEST_CASE("Timeline period_start preserves shape NaT and strided logical order",
          "[timeline-period-start]") {
  const auto matrix = pluss::timeline::make({
      {"2024-02-29T23:59:59.999999999Z", ""},
      {"1969-12-31T23:59:59.500000000Z", "2025-01-01T00:00:00Z"},
  });
  const auto projected = matrix.period_start("month");
  REQUIRE(projected.sizes().vec() == std::vector<std::int64_t>{2, 2});
  REQUIRE(formatted(projected) == std::vector<std::string>{
      "2024-02-01T00:00:00.000000000+00:00", "NaT",
      "1969-12-01T00:00:00.000000000+00:00",
      "2025-01-01T00:00:00.000000000+00:00",
  });

  auto source = pluss::timeline::make({
      "2024-01-15T12:00:00Z", "2030-01-01T00:00:00Z",
      "2024-03-20T18:00:00Z", "2030-01-01T00:00:00Z",
  });
  auto ticks = source.ticks();
  const std::array<std::int64_t, 1> sizes{2};
  const std::array<std::int64_t, 1> strides{2};
  const auto strided = pluss::Timeline::from_buffer(
      ticks.data(), sizes, strides, {}, "UTC");
  REQUIRE(formatted(strided.period_start("year")) ==
          std::vector<std::string>{
              "2024-01-01T00:00:00.000000000+00:00",
              "2024-01-01T00:00:00.000000000+00:00",
          });
}

TEST_CASE("Timeline period_start validates one canonical unit vocabulary",
          "[timeline-period-start]") {
  const auto value = pluss::timeline::make("2024-07-15T18:34:56Z");
  REQUIRE(value.period_start("YEAR").ticks() == value.period_start("year").ticks());
  REQUIRE_THROWS(value.period_start("years"));
  REQUIRE_THROWS(value.period_start("Y"));
  REQUIRE_THROWS(value.period_start("week"));
}

TEST_CASE("Timeline period_start retains DST fold identity and empty geometry",
          "[timeline-period-start]") {
  const auto folds = pluss::timeline::make({
      "2024-11-03T07:45:00Z", "2024-11-03T08:45:00Z",
  }).tz_convert("America/Edmonton");
  REQUIRE(formatted(folds.period_start("hour")) ==
          std::vector<std::string>{
              "2024-11-03T01:00:00.000000000-06:00",
              "2024-11-03T01:00:00.000000000-07:00",
          });

  const auto empty = pluss::timeline::from_epoch(
      std::vector<std::int64_t>{}, {2, 0});
  const auto projected = empty.period_start("day");
  REQUIRE(projected.sizes().vec() == std::vector<std::int64_t>{2, 0});
  REQUIRE(projected.numel() == 0);
}
