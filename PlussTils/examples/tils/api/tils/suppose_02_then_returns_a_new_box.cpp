#include <pluss/tils/suppose.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // then runs the function on the held value and returns a new box. The original keeps its value.
  auto base = tils::suppose(3);
  auto doubled = base.then([](int x) { return x * 2; });
  std::cout << "original still holds: " << base.value() << '\n';
  std::cout << "new box holds: " << doubled.value() << '\n';
  return 0;
}
