#include <pluss/tils/suppose.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // tap runs a function on the held value and returns the same box, so the value is unchanged.
  auto items = tils::suppose(std::vector<int>{1, 2, 3});
  items.tap([](const std::vector<int>& v) { std::cout << "tapped size: " << v.size() << '\n'; });
  std::cout << "value unchanged: " << items.value().size() << '\n';
  return 0;
}
