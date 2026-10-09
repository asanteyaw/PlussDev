#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // A let converts implicitly to const T&, so a function that takes a const payload accepts it.
  tils::let values = pluss::make_series(std::vector<double>{1.0, 2.0}, {2});
  auto size_of = [](const pluss::Series& s) { return s.numel(); };
  std::cout << "passed through as const: " << size_of(values) << '\n';
  return 0;
}
