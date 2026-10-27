#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cstdint>
#include <vector>

TEST_CASE("Timeline ordering is stable and places NaT last", "[timeline-ordering]") {
  const auto input = pluss::timeline::make({
      "2025-03-01T00:00:00Z", "", "2024-01-01T00:00:00Z",
      "2025-03-01T00:00:00Z", "",
  });

  REQUIRE(input.argsort().host<std::int64_t>() ==
          std::vector<std::int64_t>{2, 0, 3, 1, 4});
  REQUIRE(input.sort().ticks() ==
          std::vector<std::int64_t>{1704067200000000000LL, 1740787200000000000LL,
                                    1740787200000000000LL, pluss::Timeline::kNaT,
                                    pluss::Timeline::kNaT});
  REQUIRE(input.sort().tz() == input.tz());
  REQUIRE(input.ticks().front() == 1740787200000000000LL);
}

TEST_CASE("Timeline uniqueness retains the first occurrence", "[timeline-ordering]") {
  const auto input = pluss::timeline::make({
      "2025-03-01T00:00:00Z", "", "2024-01-01T00:00:00Z",
      "2025-03-01T00:00:00Z", "", "2024-06-01T12:00:00.123456789Z",
  }).tz_convert("America/New_York");

  const auto expected = std::vector<std::int64_t>{
      1740787200000000000LL, pluss::Timeline::kNaT, 1704067200000000000LL,
      1717243200123456789LL};
  REQUIRE(input.unique().ticks() == expected);
  REQUIRE(input.unique().tz() == "America/New_York");
}

TEST_CASE("Timeline searchsorted respects ties, NaT, and query shape", "[timeline-ordering]") {
  const auto sorted = pluss::timeline::make({
      "2024-01-01T00:00:00Z", "2024-06-01T00:00:00Z",
      "2024-06-01T00:00:00Z", "2025-01-01T00:00:00Z", "", "",
  });
  const auto queries = pluss::timeline::make({
      {"2023-12-01T00:00:00Z", "2024-06-01T00:00:00Z"},
      {"2024-08-01T00:00:00Z", ""},
  });

  const auto left = sorted.searchsorted_left(queries);
  const auto right = sorted.searchsorted_right(queries);
  REQUIRE(left.sizes().vec() == std::vector<std::int64_t>{2, 2});
  REQUIRE(right.sizes().vec() == std::vector<std::int64_t>{2, 2});
  REQUIRE(left.host<std::int64_t>() == std::vector<std::int64_t>{0, 1, 3, 4});
  REQUIRE(right.host<std::int64_t>() == std::vector<std::int64_t>{0, 3, 3, 6});
}

TEST_CASE("Timeline ordering rejects ranks outside its one-dimensional contract",
          "[timeline-ordering]") {
  const auto matrix = pluss::timeline::make({
      {"2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z"},
      {"2024-01-03T00:00:00Z", "2024-01-04T00:00:00Z"},
  });
  REQUIRE_THROWS(matrix.argsort());
  REQUIRE_THROWS(matrix.sort());
  REQUIRE_THROWS(matrix.unique());
  REQUIRE_THROWS(matrix.searchsorted_left(pluss::timeline::make("2024-01-01T00:00:00Z")));
}
