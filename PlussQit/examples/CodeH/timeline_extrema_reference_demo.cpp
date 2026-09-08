#include <pluss/pluss.h>

#include <iostream>
#include <string_view>
#include <vector>

namespace {

void show(std::string_view label, const pluss::Timeline& value) {
  std::cout << "\n=== " << label << " ===\n";
  pluss::timeline::print(std::cout, value);
  std::cout << '\n';
}

} // namespace

int main() {
  const auto observations = pluss::timeline::make({
      {"2025-06-15T08:30:00Z", ""},
      {"2024-01-02T18:45:00Z", "2026-09-30T03:15:00Z"},
  });
  show("input", observations);
  show("min", observations.min());
  show("max", observations.max());
  show("min_skipna_false", observations.min(false));
  show("max_skipna_false", observations.max(false));

  const auto duplicated = pluss::timeline::make({
      "2025-06-15T08:30:00Z", "2024-01-02T18:45:00Z",
      "2024-01-02T18:45:00Z", "2026-09-30T03:15:00Z",
      "2026-09-30T03:15:00Z",
  });
  show("duplicates_min", duplicated.min());
  show("duplicates_max", duplicated.max());

  const auto scalar = pluss::timeline::make("2025-02-03T04:05:06.123456789Z");
  show("scalar_min", scalar.min());
  show("scalar_max", scalar.max());

  const auto missing = pluss::timeline::make({"", ""});
  show("all_nat_min", missing.min());
  show("all_nat_max", missing.max());

  const auto empty = pluss::timeline::from_epoch(std::vector<std::int64_t>{});
  show("empty_min", empty.min());
  show("empty_max", empty.max());

  const auto local = observations.tz_convert("America/New_York");
  show("new_york_min", local.min());
  show("new_york_max", local.max());

  std::cout << "\n=== reduction_contract ===\n" << std::boolalpha
            << "min shape=[]: " << observations.min().is_scalar() << '\n'
            << "max shape=[]: " << observations.max().is_scalar() << '\n'
            << "timezone retained: "
            << (local.min().tz() == local.tz() && local.max().tz() == local.tz()) << '\n'
            << "nanoseconds retained: "
            << (scalar.min().ticks() == scalar.ticks() &&
                scalar.max().ticks() == scalar.ticks()) << '\n'
            << "receiver unchanged: " << (observations.sizes().vec() == std::vector<std::int64_t>{2, 2}) << '\n';
}
