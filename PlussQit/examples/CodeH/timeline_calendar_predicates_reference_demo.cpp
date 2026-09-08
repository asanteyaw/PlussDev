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

  const auto utc = pluss::timeline::make("2021-01-01T00:30:00Z");
  const auto local = utc.tz_convert("America/New_York");
  std::cout << "\n=== timezone_year_boundary ===\n" << std::boolalpha
            << "UTC: " << utc.isoformat()
            << ", year_start=" << utc.is_year_start().item<bool>()
            << ", year_end=" << utc.is_year_end().item<bool>() << '\n'
            << "New York: " << local.isoformat()
            << ", year_start=" << local.is_year_start().item<bool>()
            << ", year_end=" << local.is_year_end().item<bool>() << '\n';

  const auto matrix = pluss::timeline::make({
      {"2024-03-04T12:00:00Z", "2024-03-09T12:00:00Z"},
      {"", "2024-03-10T12:00:00Z"},
  });
  show("weekend_preserves_shape", matrix.is_weekend());

  std::cout << "\n=== predicate_identity ===\n"
            << "weekday partition="
            << week.is_monday().logical_or(week.is_tuesday())
                   .logical_or(week.is_wednesday()).logical_or(week.is_thursday())
                   .logical_or(week.is_friday()).logical_or(week.is_saturday())
                   .logical_or(week.is_sunday()).equal(week.isvalid()) << '\n'
            << "weekend/business complement="
            << boundaries.is_weekend().logical_xor(boundaries.is_business_day())
                   .equal(boundaries.isvalid()) << '\n';
}
