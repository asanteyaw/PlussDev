#include <pluss/pluss.h>

#include <iostream>
#include <string_view>

namespace {
void show(std::string_view label, const pluss::Timeline& value) {
  std::cout << "\n=== " << label << " ===\n" << pluss::timeline::print(value) << '\n';
}

void show(std::string_view label, const pluss::Series& value) {
  std::cout << "\n=== " << label << " ===\n" << pluss::series::print(value) << '\n';
}
} // namespace

int main() {
  const auto input = pluss::timeline::make({
      "2025-03-01T00:00:00Z", "", "2024-01-01T00:00:00Z",
      "2025-03-01T00:00:00Z", "", "2024-06-01T12:00:00.123456789Z",
  });
  show("input", input);
  show("argsort", input.argsort());
  show("sort", input.sort());
  show("unique", input.unique());

  const auto sorted = pluss::timeline::make({
      "2024-01-01T00:00:00Z", "2024-06-01T00:00:00Z",
      "2024-06-01T00:00:00Z", "2025-01-01T00:00:00Z", "", "",
  });
  const auto queries = pluss::timeline::make({
      {"2023-12-01T00:00:00Z", "2024-06-01T00:00:00Z"},
      {"2024-08-01T00:00:00Z", ""},
  });
  show("sorted", sorted);
  show("queries", queries);
  show("searchsorted_left", sorted.searchsorted_left(queries));
  show("searchsorted_right", sorted.searchsorted_right(queries));

  const auto duplicate_key = pluss::timeline::make("2024-06-01T00:00:00Z");
  const auto begin = sorted.searchsorted_left(duplicate_key).item<std::int64_t>();
  const auto end = sorted.searchsorted_right(duplicate_key).item<std::int64_t>();
  std::cout << "\n=== equal_range ===\nbegin=" << begin << ", end=" << end
            << ", count=" << (end - begin) << '\n';

  const auto absent_key = pluss::timeline::make("2024-07-01T00:00:00Z");
  const auto absent_left = sorted.searchsorted_left(absent_key).item<std::int64_t>();
  const auto absent_right = sorted.searchsorted_right(absent_key).item<std::int64_t>();
  std::cout << "\n=== exact_membership ===\n" << std::boolalpha
            << "found=" << (absent_right > absent_left) << '\n';

  show("sorted_unique", input.sort().unique());

  const auto local = input.tz_convert("America/New_York");
  show("local_unique", local.unique());
}
