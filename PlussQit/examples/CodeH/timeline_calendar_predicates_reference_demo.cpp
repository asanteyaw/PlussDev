#include <pluss/pluss.h>

#include <iostream>
#include <string_view>

namespace {

void show(std::string_view label, const pluss::Series& values) {
  std::cout << "\n=== " << label << " ===\n";
  pluss::series::print(std::cout, values);
  std::cout << '\n';
}

} // namespace

int main() {
  const auto boundaries = pluss::timeline::make({
      "2019-12-31T12:00:00Z",
      "2020-01-01T12:00:00Z",
      "2020-02-29T12:00:00Z",
      "2020-03-31T12:00:00Z",
      "2020-04-01T12:00:00Z",
      "2020-12-31T12:00:00Z",
      "2021-01-01T12:00:00Z",
      "2024-02-29T12:00:00Z",
      "",
  });

  show("is_leap_year", boundaries.is_leap_year());
  show("is_month_start", boundaries.is_month_start());
  show("is_month_end", boundaries.is_month_end());
  show("is_quarter_start", boundaries.is_quarter_start());
  show("is_quarter_end", boundaries.is_quarter_end());
  show("is_year_start", boundaries.is_year_start());
  show("is_year_end", boundaries.is_year_end());
  show("is_weekend", boundaries.is_weekend());
  show("is_business_day", boundaries.is_business_day());

  const auto week = pluss::timeline::make({
      "2024-03-04T12:00:00Z", "2024-03-05T12:00:00Z",
      "2024-03-06T12:00:00Z", "2024-03-07T12:00:00Z",
      "2024-03-08T12:00:00Z", "2024-03-09T12:00:00Z",
      "2024-03-10T12:00:00Z", "",
  });
  show("is_monday", week.is_monday());
  show("is_tuesday", week.is_tuesday());
  show("is_wednesday", week.is_wednesday());
  show("is_thursday", week.is_thursday());
  show("is_friday", week.is_friday());
  show("is_saturday", week.is_saturday());
  show("is_sunday", week.is_sunday());
  show("tuesday_or_thursday",
       week.is_tuesday().logical_or(week.is_thursday()));

  const auto week_matrix = pluss::timeline::make({
      {"2024-03-04T12:00:00Z", "2024-03-05T12:00:00Z",
       "2024-03-06T12:00:00Z", "2024-03-07T12:00:00Z"},
      {"2024-03-08T12:00:00Z", "2024-03-09T12:00:00Z",
       "2024-03-10T12:00:00Z", ""},
  });
  show("matrix_named_monday", week_matrix.is_monday());
  show("matrix_named_tuesday", week_matrix.is_tuesday());
  show("matrix_named_wednesday", week_matrix.is_wednesday());
  show("matrix_named_thursday", week_matrix.is_thursday());
  show("matrix_named_friday", week_matrix.is_friday());
  show("matrix_named_saturday", week_matrix.is_saturday());
  show("matrix_named_sunday", week_matrix.is_sunday());

  const auto weekday_policy = pluss::timeline::make({
      "2024-12-25T12:00:00Z",
      "2024-12-28T12:00:00Z",
  });
  show("weekday_only_business_rule", weekday_policy.is_business_day());

  const auto utc = pluss::timeline::make("2021-01-01T00:30:00Z");
  const auto local = utc.tz_convert("America/New_York");
  std::cout << "\n=== timezone_year_boundary ===\n" << std::boolalpha
            << "UTC: " << utc.isoformat()
            << ", year_start=" << utc.is_year_start().item<bool>()
            << ", year_end=" << utc.is_year_end().item<bool>() << '\n'
            << "New York: " << local.isoformat()
            << ", year_start=" << local.is_year_start().item<bool>()
            << ", year_end=" << local.is_year_end().item<bool>() << '\n';

  const auto century_years = pluss::timeline::make({
      "1900-06-15T12:00:00Z",
      "2000-06-15T12:00:00Z",
      "2100-06-15T12:00:00Z",
  });
  show("century_leap_rules", century_years.is_leap_year());

  const auto boundary_matrix = pluss::timeline::make({
      {"2024-01-01T23:59:59.999999999Z", "2024-01-02T00:00:00Z",
       "2024-02-29T00:00:00Z"},
      {"2024-03-31T12:00:00Z", "2024-04-01T00:00:00Z",
       "2024-12-31T00:00:00Z"},
  });
  show("matrix_month_start", boundary_matrix.is_month_start());
  show("matrix_month_end", boundary_matrix.is_month_end());
  show("matrix_quarter_start", boundary_matrix.is_quarter_start());
  show("matrix_quarter_end", boundary_matrix.is_quarter_end());
  show("matrix_year_start", boundary_matrix.is_year_start());
  show("matrix_year_end", boundary_matrix.is_year_end());

  std::cout << "\n=== timezone_boundary_family ===\n"
            << "UTC: leap=" << utc.is_leap_year().item<bool>()
            << ", month_start=" << utc.is_month_start().item<bool>()
            << ", month_end=" << utc.is_month_end().item<bool>()
            << ", quarter_start=" << utc.is_quarter_start().item<bool>()
            << ", quarter_end=" << utc.is_quarter_end().item<bool>()
            << ", year_start=" << utc.is_year_start().item<bool>()
            << ", year_end=" << utc.is_year_end().item<bool>() << '\n'
            << "New York: leap=" << local.is_leap_year().item<bool>()
            << ", month_start=" << local.is_month_start().item<bool>()
            << ", month_end=" << local.is_month_end().item<bool>()
            << ", quarter_start=" << local.is_quarter_start().item<bool>()
            << ", quarter_end=" << local.is_quarter_end().item<bool>()
            << ", year_start=" << local.is_year_start().item<bool>()
            << ", year_end=" << local.is_year_end().item<bool>() << '\n';

  const auto scalar = pluss::timeline::make("2024-02-29T23:59:59.999999999Z");
  show("scalar_leap", scalar.is_leap_year());
  show("scalar_month_end", scalar.is_month_end());

  const auto empty = pluss::timeline::full({0}, scalar);
  show("empty_month_start", empty.is_month_start());
  show("empty_year_end", empty.is_year_end());

  const auto matrix = pluss::timeline::make({
      {"2024-03-04T12:00:00Z", "2024-03-09T12:00:00Z"},
      {"", "2024-03-10T12:00:00Z"},
  });
  show("weekend_preserves_shape", matrix.is_weekend());
  show("matrix_monday", matrix.is_monday());
  show("matrix_saturday", matrix.is_saturday());
  show("matrix_sunday", matrix.is_sunday());

  const auto utc_weekend = pluss::timeline::make("2024-03-09T01:30:00Z");
  const auto new_york_weekday = utc_weekend.tz_convert("America/New_York");
  std::cout << "\n=== timezone_weekday_flip ===\n"
            << "UTC: friday=" << utc_weekend.is_friday().item<bool>()
            << ", saturday=" << utc_weekend.is_saturday().item<bool>()
            << ", weekend=" << utc_weekend.is_weekend().item<bool>()
            << ", business=" << utc_weekend.is_business_day().item<bool>() << '\n'
            << "New York: friday=" << new_york_weekday.is_friday().item<bool>()
            << ", saturday=" << new_york_weekday.is_saturday().item<bool>()
            << ", weekend=" << new_york_weekday.is_weekend().item<bool>()
            << ", business=" << new_york_weekday.is_business_day().item<bool>() << '\n';

  show("scalar_friday", new_york_weekday.is_friday());
  const auto empty_week = pluss::timeline::full({0}, new_york_weekday);
  show("empty_business_day", empty_week.is_business_day());
  show("empty_weekend", empty_week.is_weekend());
  show("empty_sunday", empty_week.is_sunday());

  std::cout << "\n=== predicate_identity ===\n"
            << "weekday partition="
            << week.is_monday().logical_or(week.is_tuesday())
                   .logical_or(week.is_wednesday()).logical_or(week.is_thursday())
                   .logical_or(week.is_friday()).logical_or(week.is_saturday())
                   .logical_or(week.is_sunday()).equal(week.isvalid()) << '\n'
            << "weekend/business complement="
            << boundaries.is_weekend().logical_xor(boundaries.is_business_day())
                   .equal(boundaries.isvalid()) << '\n'
            << "weekend is Saturday or Sunday="
            << week.is_saturday().logical_or(week.is_sunday())
                   .equal(week.is_weekend()) << '\n'
            << "business day is Monday through Friday="
            << week.is_monday().logical_or(week.is_tuesday())
                   .logical_or(week.is_wednesday()).logical_or(week.is_thursday())
                   .logical_or(week.is_friday()).equal(week.is_business_day()) << '\n'
            << "Monday code 0=" << week.is_monday().equal(week.weekday().eq(0)) << '\n'
            << "Tuesday code 1=" << week.is_tuesday().equal(week.weekday().eq(1)) << '\n'
            << "Wednesday code 2=" << week.is_wednesday().equal(week.weekday().eq(2)) << '\n'
            << "Thursday code 3=" << week.is_thursday().equal(week.weekday().eq(3)) << '\n'
            << "Friday code 4=" << week.is_friday().equal(week.weekday().eq(4)) << '\n'
            << "Saturday code 5=" << week.is_saturday().equal(week.weekday().eq(5)) << '\n'
            << "Sunday code 6=" << week.is_sunday().equal(week.weekday().eq(6)) << '\n'
            << "quarter starts are month starts="
            << boundary_matrix.is_quarter_start()
                   .logical_and(boundary_matrix.is_month_start())
                   .equal(boundary_matrix.is_quarter_start()) << '\n'
            << "quarter ends are month ends="
            << boundary_matrix.is_quarter_end()
                   .logical_and(boundary_matrix.is_month_end())
                   .equal(boundary_matrix.is_quarter_end()) << '\n'
            << "year starts are quarter starts="
            << boundary_matrix.is_year_start()
                   .logical_and(boundary_matrix.is_quarter_start())
                   .equal(boundary_matrix.is_year_start()) << '\n'
            << "year ends are quarter ends="
            << boundary_matrix.is_year_end()
                   .logical_and(boundary_matrix.is_quarter_end())
                   .equal(boundary_matrix.is_year_end()) << '\n';
}
