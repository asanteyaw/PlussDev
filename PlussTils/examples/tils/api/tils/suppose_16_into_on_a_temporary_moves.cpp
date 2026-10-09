#include <pluss/tils/suppose.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // into on a temporary box converts by moving, so the source value is not copied first.
  auto text = tils::suppose(std::string("12345")).into<std::string>();
  std::cout << "moved into a box of the same type: " << text.value() << '\n';
  return 0;
}
