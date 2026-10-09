#include <pluss/tils/suppose.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // A chain can mix tap and then. Each tap sees the value, and the final then decides the result.
  auto start = tils::suppose(3);
  auto result = start.tap([](const int& x) { std::cout << "seen: " << x << '\n'; })
                     .then([](const int& x) { return x * x; });
  std::cout << "squared: " << result.value() << '\n';
  return 0;
}
