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
  REQUIRE(bools(t.is_tuesday().logical_or(t.is_thursday())) ==
          std::vector<bool>{false, true, false, true, false, false, false, false});

  REQUIRE(t.is_saturday().logical_or(t.is_sunday()).equal(t.is_weekend()));
  REQUIRE(t.is_monday().logical_or(t.is_tuesday())
              .logical_or(t.is_wednesday()).logical_or(t.is_thursday())
              .logical_or(t.is_friday()).equal(t.is_business_day()));
  REQUIRE(t.is_monday().equal(t.weekday().eq(0)));
  REQUIRE(t.is_tuesday().equal(t.weekday().eq(1)));
  REQUIRE(t.is_wednesday().equal(t.weekday().eq(2)));
  REQUIRE(t.is_thursday().equal(t.weekday().eq(3)));
  REQUIRE(t.is_friday().equal(t.weekday().eq(4)));
  REQUIRE(t.is_saturday().equal(t.weekday().eq(5)));
  REQUIRE(t.is_sunday().equal(t.weekday().eq(6)));

  const auto matrix = pluss::timeline::make({
      {"2024-03-04T12:00:00Z", "2024-03-05T12:00:00Z",
       "2024-03-06T12:00:00Z", "2024-03-07T12:00:00Z"},
      {"2024-03-08T12:00:00Z", "2024-03-09T12:00:00Z",
       "2024-03-10T12:00:00Z", ""},
  });
  REQUIRE(matrix.is_monday().sizes().vec() == std::vector<std::int64_t>{2, 4});
  REQUIRE(bools(matrix.is_monday()) == expected[0]);
  REQUIRE(bools(matrix.is_tuesday()) == expected[1]);
  REQUIRE(bools(matrix.is_wednesday()) == expected[2]);
  REQUIRE(bools(matrix.is_thursday()) == expected[3]);
  REQUIRE(bools(matrix.is_friday()) == expected[4]);
  REQUIRE(bools(matrix.is_saturday()) == expected[5]);
  REQUIRE(bools(matrix.is_sunday()) == expected[6]);
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
  REQUIRE(bools(matrix.is_monday()) == std::vector<bool>{true, false, false, false});
  REQUIRE(bools(matrix.is_saturday()) == std::vector<bool>{false, true, false, false});
  REQUIRE(bools(matrix.is_sunday()) == std::vector<bool>{false, false, false, true});
}

TEST_CASE("Timeline weekday predicates preserve scalar and empty geometry",
          "[timeline-calendar-predicates]") {
  const auto utc = pluss::timeline::make("2024-03-09T01:30:00Z");
  const auto local = utc.tz_convert("America/New_York");

  REQUIRE_FALSE(utc.is_friday().item<bool>());
  REQUIRE(utc.is_saturday().item<bool>());
  REQUIRE(utc.is_weekend().item<bool>());
  REQUIRE_FALSE(utc.is_business_day().item<bool>());
  REQUIRE(local.is_friday().item<bool>());
  REQUIRE_FALSE(local.is_saturday().item<bool>());
  REQUIRE_FALSE(local.is_weekend().item<bool>());
  REQUIRE(local.is_business_day().item<bool>());
  REQUIRE(local.is_friday().dim() == 0);

  const auto empty = pluss::timeline::full({0}, local);
  REQUIRE(empty.is_business_day().sizes().vec() == std::vector<std::int64_t>{0});
  REQUIRE(empty.is_weekend().sizes().vec() == std::vector<std::int64_t>{0});
  REQUIRE(empty.is_sunday().sizes().vec() == std::vector<std::int64_t>{0});
}

TEST_CASE("Timeline built-in business predicate is weekday-only",
          "[timeline-calendar-predicates]") {
  const auto dates = pluss::timeline::make({
      "2024-12-25T12:00:00Z",
      "2024-12-28T12:00:00Z",
  });
  REQUIRE(bools(dates.is_business_day()) == std::vector<bool>{true, false});
}

TEST_CASE("Timeline boundary predicates cover civil rules and every rank",
          "[timeline-calendar-predicates]") {
  const auto centuries = pluss::timeline::make({
      "1900-06-15T12:00:00Z",
      "2000-06-15T12:00:00Z",
      "2100-06-15T12:00:00Z",
  });
  REQUIRE(bools(centuries.is_leap_year()) ==
          std::vector<bool>{false, true, false});

  const auto matrix = pluss::timeline::make({
      {"2024-01-01T23:59:59.999999999Z", "2024-01-02T00:00:00Z",
       "2024-02-29T00:00:00Z"},
      {"2024-03-31T12:00:00Z", "2024-04-01T00:00:00Z",
       "2024-12-31T00:00:00Z"},
  });
  const std::vector<std::int64_t> shape{2, 3};
  REQUIRE(matrix.is_month_start().sizes().vec() == shape);
  REQUIRE(matrix.is_month_end().sizes().vec() == shape);
  REQUIRE(matrix.is_quarter_start().sizes().vec() == shape);
  REQUIRE(matrix.is_quarter_end().sizes().vec() == shape);
  REQUIRE(matrix.is_year_start().sizes().vec() == shape);
  REQUIRE(matrix.is_year_end().sizes().vec() == shape);
  REQUIRE(bools(matrix.is_month_start()) ==
          std::vector<bool>{true, false, false, false, true, false});
  REQUIRE(bools(matrix.is_month_end()) ==
          std::vector<bool>{false, false, true, true, false, true});
  REQUIRE(bools(matrix.is_quarter_start()) ==
          std::vector<bool>{true, false, false, false, true, false});
  REQUIRE(bools(matrix.is_quarter_end()) ==
          std::vector<bool>{false, false, false, true, false, true});
  REQUIRE(bools(matrix.is_year_start()) ==
          std::vector<bool>{true, false, false, false, false, false});
  REQUIRE(bools(matrix.is_year_end()) ==
          std::vector<bool>{false, false, false, false, false, true});

  const auto scalar = pluss::timeline::make("2024-02-29T23:59:59.999999999Z");
  REQUIRE(scalar.is_leap_year().dim() == 0);
  REQUIRE(scalar.is_month_end().dim() == 0);
  REQUIRE(scalar.is_leap_year().item<bool>());
  REQUIRE(scalar.is_month_end().item<bool>());

  const auto empty = pluss::timeline::full({0}, scalar);
  REQUIRE(empty.is_month_start().sizes().vec() == std::vector<std::int64_t>{0});
  REQUIRE(empty.is_year_end().sizes().vec() == std::vector<std::int64_t>{0});
}

TEST_CASE("Timeline timezone conversion can invert every New Year boundary predicate",
          "[timeline-calendar-predicates]") {
  const auto utc = pluss::timeline::make("2021-01-01T00:30:00Z");
  const auto local = utc.tz_convert("America/New_York");

  REQUIRE_FALSE(utc.is_leap_year().item<bool>());
  REQUIRE(utc.is_month_start().item<bool>());
  REQUIRE_FALSE(utc.is_month_end().item<bool>());
  REQUIRE(utc.is_quarter_start().item<bool>());
  REQUIRE_FALSE(utc.is_quarter_end().item<bool>());
  REQUIRE(utc.is_year_start().item<bool>());
  REQUIRE_FALSE(utc.is_year_end().item<bool>());

  REQUIRE(local.is_leap_year().item<bool>());
  REQUIRE_FALSE(local.is_month_start().item<bool>());
  REQUIRE(local.is_month_end().item<bool>());
  REQUIRE_FALSE(local.is_quarter_start().item<bool>());
  REQUIRE(local.is_quarter_end().item<bool>());
  REQUIRE_FALSE(local.is_year_start().item<bool>());
  REQUIRE(local.is_year_end().item<bool>());
}
