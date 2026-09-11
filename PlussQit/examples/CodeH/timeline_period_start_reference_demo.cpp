#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

void show(std::string_view label, const pluss::Timeline& value) {
  std::cout << "\n=== " << label << " ===\n"
            << pluss::note::print(
                   pluss::timeline::format(value, "%FT%T.%f%z"))
            << '\n';
}

void reject(std::string_view label, std::string_view unit,
            const pluss::Timeline& value) {
  try {
    (void)value.period_start(unit);
  } catch (const pluss::Error&) {
    std::cout << "\n=== " << label << " ===\nrejected\n";
  }
}

} // namespace

int main() {
  const auto value = pluss::timeline::make(
      "2024-07-15T18:34:56.123456789Z").tz_convert("America/Edmonton");
  show("source", value);
  show("year", value.period_start("year"));
  show("month", value.period_start("month"));
  show("day", value.period_start("day"));
  show("hour", value.period_start("hour"));
  show("minute", value.period_start("minute"));
  show("second", value.period_start("second"));
  const auto once = value.period_start("month");
  const auto twice = once.period_start("month");
  std::cout << "\n=== idempotent ===\n"
            << std::boolalpha << once.eq(twice).all().item<bool>() << '\n';

  const auto matrix = pluss::timeline::make({
      {"2024-02-29T23:59:59.999999999Z", ""},
      {"1969-12-31T23:59:59.500000000Z", "2025-01-01T00:00:00Z"},
  });
  show("matrix_month", matrix.period_start("month"));

  auto storage = pluss::timeline::make({
      "2024-01-15T12:00:00Z", "2030-01-01T00:00:00Z",
      "2024-03-20T18:00:00Z", "2030-01-01T00:00:00Z",
  }).ticks();
  const std::array<std::int64_t, 1> sizes{2};
  const std::array<std::int64_t, 1> strides{2};
  const auto strided = pluss::Timeline::from_buffer(
      storage.data(), sizes, strides, {}, "UTC");
  show("strided_year", strided.period_start("year"));

  const auto folds = pluss::timeline::make({
      "2024-11-03T07:45:00Z", "2024-11-03T08:45:00Z",
  }).tz_convert("America/Edmonton");
  show("fold_hour", folds.period_start("hour"));

  const auto empty = pluss::timeline::from_epoch(
      std::vector<std::int64_t>{}, {2, 0});
  show("empty_day", empty.period_start("day"));

  reject("plural_alias", "years", value);
  reject("unsupported_unit", "week", value);
}
