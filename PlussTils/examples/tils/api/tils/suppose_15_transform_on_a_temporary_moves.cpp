#include <pluss/tils/suppose.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // transform on a temporary box moves the value into the function, so a large value is not copied.
  auto moved = tils::suppose(std::vector<int>{1, 2, 3}).transform([](std::vector<int> v) { v.push_back(4); return v; });
  std::cout << "size after transform on a temporary: " << moved.value().size() << '\n';
  return 0;
}
