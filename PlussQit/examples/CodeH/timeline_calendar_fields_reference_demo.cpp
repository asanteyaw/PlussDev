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
  const auto values = pluss::timeline::make({
      "2020-12-31T23:59:59.123456789Z",
      "2021-01-01T00:00:00.987654321Z",
      "2024-02-29T06:07:08.009010011Z",
      "",
  });

  show("year", values.year());
  show("month", values.month());
  show("day", values.day());
  show("hour", values.hour());
  show("minute", values.minute());
  show("second", values.second());
  show("nanosecond", values.nanosecond());
  show("microsecond", values.microsecond());
  show("millisecond", values.millisecond());
  show("weekday", values.weekday());
  show("dayofyear", values.dayofyear());
  show("quarter", values.quarter());
  show("week", values.week());

  const auto utc = pluss::timeline::make("2024-03-01T00:30:00Z");
  const auto new_york = utc.tz_convert("America/New_York");
  std::cout << "\n=== timezone_boundary ===\n";
  std::cout << "UTC: " << utc.isoformat()
            << ", month=" << utc.month().item<std::int32_t>()
            << ", day=" << utc.day().item<std::int32_t>()
            << ", hour=" << utc.hour().item<std::int32_t>()
            << ", dayofyear=" << utc.dayofyear().item<std::int32_t>() << '\n';
  std::cout << "New York: " << new_york.isoformat()
            << ", month=" << new_york.month().item<std::int32_t>()
            << ", day=" << new_york.day().item<std::int32_t>()
            << ", hour=" << new_york.hour().item<std::int32_t>()
            << ", dayofyear=" << new_york.dayofyear().item<std::int32_t>() << '\n';

  const auto matrix = pluss::timeline::make({
      {"2024-01-01T00:00:00Z", "2024-04-01T00:00:00Z"},
      {"2024-07-01T00:00:00Z", "2024-10-01T00:00:00Z"},
  });
  show("quarter_preserves_shape", matrix.quarter());

  std::cout << "\n=== aliases ===\n" << std::boolalpha
            << "year aliases=" << matrix.year().equal(matrix.y()) << '\n'
            << "weekday aliases=" << matrix.weekday().equal(matrix.dayofweek()) << '\n'
            << "week aliases=" << matrix.week().equal(matrix.weekofyear()) << '\n';
}
