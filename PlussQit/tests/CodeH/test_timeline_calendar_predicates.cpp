#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <vector>

namespace {

std::vector<bool> bools(const pluss::Series& value) {
  std::vector<bool> out;
  const auto flat = value.reshape({value.numel()});
  for (std::int64_t i = 0; i < flat.numel(); ++i) {
    out.push_back(flat.index({i}).item<bool>());
  }
  return out;
}

} // namespace

TEST_CASE("Timeline calendar boundaries and NaT have exact predicate semantics",
          "[timeline-calendar-predicates]") {
  const auto t = pluss::timeline::make({
      "2019-12-31T12:00:00Z", "2020-01-01T12:00:00Z",
      "2020-02-29T12:00:00Z", "2020-03-31T12:00:00Z",
      "2020-04-01T12:00:00Z", "2020-12-31T12:00:00Z",
      "2021-01-01T12:00:00Z", "2024-02-29T12:00:00Z", "",
  });

  REQUIRE(bools(t.is_leap_year()) == std::vector<bool>{false, true, true, true, true, true, false, true, false});
  REQUIRE(bools(t.is_month_start()) == std::vector<bool>{false, true, false, false, true, false, true, false, false});
  REQUIRE(bools(t.is_month_end()) == std::vector<bool>{true, false, true, true, false, true, false, true, false});
  REQUIRE(bools(t.is_quarter_start()) == std::vector<bool>{false, true, false, false, true, false, true, false, false});
  REQUIRE(bools(t.is_quarter_end()) == std::vector<bool>{true, false, false, true, false, true, false, false, false});
  REQUIRE(bools(t.is_year_start()) == std::vector<bool>{false, true, false, false, false, false, true, false, false});
  REQUIRE(bools(t.is_year_end()) == std::vector<bool>{true, false, false, false, false, true, false, false, false});
  REQUIRE(bools(t.is_weekend()) == std::vector<bool>{false, false, true, false, false, false, false, false, false});
  REQUIRE(bools(t.is_business_day()) == std::vector<bool>{true, true, false, true, true, true, true, true, false});
}

TEST_CASE("Timeline named weekdays partition valid dates", "[timeline-calendar-predicates]") {
  const auto t = pluss::timeline::make({
      "2024-03-04T12:00:00Z", "2024-03-05T12:00:00Z",
      "2024-03-06T12:00:00Z", "2024-03-07T12:00:00Z",
      "2024-03-08T12:00:00Z", "2024-03-09T12:00:00Z",
      "2024-03-10T12:00:00Z", "",
  });
  const std::vector<std::vector<bool>> expected{
      {true, false, false, false, false, false, false, false},
      {false, true, false, false, false, false, false, false},
      {false, false, true, false, false, false, false, false},
      {false, false, false, true, false, false, false, false},
      {false, false, false, false, true, false, false, false},
      {false, false, false, false, false, true, false, false},
      {false, false, false, false, false, false, true, false},
  };
  REQUIRE(bools(t.is_monday()) == expected[0]);
  REQUIRE(bools(t.is_tuesday()) == expected[1]);
  REQUIRE(bools(t.is_wednesday()) == expected[2]);
  REQUIRE(bools(t.is_thursday()) == expected[3]);
  REQUIRE(bools(t.is_friday()) == expected[4]);
  REQUIRE(bools(t.is_saturday()) == expected[5]);
  REQUIRE(bools(t.is_sunday()) == expected[6]);
}

TEST_CASE("Timeline predicates use local dates and preserve shape",
          "[timeline-calendar-predicates]") {
  const auto utc = pluss::timeline::make("2021-01-01T00:30:00Z");
  const auto local = utc.tz_convert("America/New_York");
  REQUIRE(utc.is_year_start().item<bool>());
  REQUIRE_FALSE(utc.is_year_end().item<bool>());
  REQUIRE_FALSE(local.is_year_start().item<bool>());
  REQUIRE(local.is_year_end().item<bool>());

  const auto matrix = pluss::timeline::make({
      {"2024-03-04T12:00:00Z", "2024-03-09T12:00:00Z"},
      {"", "2024-03-10T12:00:00Z"},
  });
  REQUIRE(matrix.is_weekend().sizes().vec() == std::vector<std::int64_t>{2, 2});
  REQUIRE(bools(matrix.is_weekend()) == std::vector<bool>{false, true, false, true});
}
