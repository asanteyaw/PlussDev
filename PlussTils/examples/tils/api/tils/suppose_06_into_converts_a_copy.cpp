#include <pluss/tils/suppose.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // into<U> converts the held value to U and returns a box of U. The original is kept.
  const auto ratio = tils::suppose(3.7);
  const auto whole = ratio.into<int>();
  std::cout << "converted: " << whole.value() << '\n';
  std::cout << "original still holds: " << ratio.value() << '\n';
  return 0;
}
