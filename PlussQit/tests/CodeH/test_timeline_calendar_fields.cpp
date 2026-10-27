#include <pluss/pluss.h>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <vector>

namespace {

template <class T>
std::vector<T> values_of(const pluss::Series& values) {
  return values.host<T>().array().data;
}

} // namespace

TEST_CASE("Timeline calendar fields honor ranges missingness and ISO boundaries", "[timeline-calendar-fields]") {
  const auto values = pluss::timeline::make({
      "2020-12-31T23:59:59.123456789Z",
      "2021-01-01T00:00:00.987654321Z",
      "2024-02-29T06:07:08.009010011Z",
      "",
  });

  REQUIRE(values_of<std::int32_t>(values.year()) == std::vector<std::int32_t>{2020, 2021, 2024, -1});
  REQUIRE(values_of<std::int32_t>(values.month()) == std::vector<std::int32_t>{12, 1, 2, -1});
  REQUIRE(values_of<std::int32_t>(values.day()) == std::vector<std::int32_t>{31, 1, 29, -1});
  REQUIRE(values_of<std::int32_t>(values.hour()) == std::vector<std::int32_t>{23, 0, 6, -1});
  REQUIRE(values_of<std::int32_t>(values.minute()) == std::vector<std::int32_t>{59, 0, 7, -1});
  REQUIRE(values_of<std::int32_t>(values.second()) == std::vector<std::int32_t>{59, 0, 8, -1});
  REQUIRE(values_of<std::int64_t>(values.nanosecond()) == std::vector<std::int64_t>{123456789, 987654321, 9010011, -1});
  REQUIRE(values_of<std::int32_t>(values.microsecond()) == std::vector<std::int32_t>{123456, 987654, 9010, -1});
  REQUIRE(values_of<std::int32_t>(values.millisecond()) == std::vector<std::int32_t>{123, 987, 9, -1});
  REQUIRE(values_of<std::int32_t>(values.weekday()) == std::vector<std::int32_t>{3, 4, 3, -1});
  REQUIRE(values_of<std::int32_t>(values.dayofyear()) == std::vector<std::int32_t>{366, 1, 60, -1});
  REQUIRE(values_of<std::int32_t>(values.quarter()) == std::vector<std::int32_t>{4, 1, 1, -1});
  REQUIRE(values_of<std::int32_t>(values.week()) == std::vector<std::int32_t>{53, 53, 9, -1});
}

TEST_CASE("Timeline calendar fields use presentation timezone", "[timeline-calendar-fields]") {
  const auto utc = pluss::timeline::make("2024-03-01T00:30:00Z");
  const auto local = utc.tz_convert("America/New_York");

  REQUIRE(utc.ticks() == local.ticks());
  REQUIRE(utc.month().item<std::int32_t>() == 3);
  REQUIRE(utc.day().item<std::int32_t>() == 1);
  REQUIRE(utc.hour().item<std::int32_t>() == 0);
  REQUIRE(utc.dayofyear().item<std::int32_t>() == 61);
  REQUIRE(local.month().item<std::int32_t>() == 2);
  REQUIRE(local.day().item<std::int32_t>() == 29);
  REQUIRE(local.hour().item<std::int32_t>() == 19);
  REQUIRE(local.dayofyear().item<std::int32_t>() == 60);
}

TEST_CASE("Timeline calendar fields preserve shape", "[timeline-calendar-fields]") {
  const auto matrix = pluss::timeline::make({
      {"2024-01-01T00:00:00Z", "2024-04-01T00:00:00Z"},
      {"2024-07-01T00:00:00Z", "2024-10-01T00:00:00Z"},
  });

  REQUIRE(matrix.quarter().sizes().vec() == std::vector<std::int64_t>{2, 2});
  REQUIRE(values_of<std::int32_t>(matrix.quarter()) == std::vector<std::int32_t>{1, 2, 3, 4});
}
