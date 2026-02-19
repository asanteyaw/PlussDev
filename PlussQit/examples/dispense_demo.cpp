#include <pluss/pluss.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

int main() {
  std::cout << "=== pluss::dispense demo (dynamic method call) ===\n\n";

  const auto s = pluss::value({-4.0, 5.0, -6.0});
  const auto s_abs = pluss::dispense<pluss::Series>(s, "abs");
  std::cout << "dispense<Series>(Series, \"abs\")[0]: "
            << s_abs.to_vec<double>(0) << "\n";

  const auto n = pluss::content({"apple", "banana", "apricot"});
  const auto starts_with_a = pluss::dispense<std::vector<std::uint8_t>>(n, "starts_with", "a");
  std::cout << "dispense<vector<uint8_t>>(Note, \"starts_with\", \"a\"): ["
            << static_cast<int>(starts_with_a[0]) << ", "
            << static_cast<int>(starts_with_a[1]) << ", "
            << static_cast<int>(starts_with_a[2]) << "]\n";

  const auto t = pluss::moment("2024-01-15T10:11:12Z");
  const auto floor_d = pluss::dispense<pluss::codeh::Timeline>(t, "floor", "D");
  std::cout << "dispense<Timeline>(Timeline, \"floor\", \"D\") hour: "
            << floor_d.hour().to_vec<std::int32_t>(0) << "\n";

  const auto year = pluss::dispense_any(t, "year");
  if (year.is<pluss::Series>()) {
    std::cout << "dispense_any(Timeline, \"year\") -> Series value: "
              << year.as<pluss::Series>().to_vec<std::int32_t>(0) << "\n";
  }

  const pluss::Queue qn = n;
  const auto qlower = pluss::dispense_any(qn, "lower");
  if (qlower.is<pluss::Note>()) {
    std::cout << "dispense_any(Queue, \"lower\") -> Note[0]: "
              << qlower.as<pluss::Note>().at(0) << "\n";
  }

  std::cout << "\nDone.\n";
  return 0;
}
