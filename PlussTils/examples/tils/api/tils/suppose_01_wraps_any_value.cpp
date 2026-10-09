#include <pluss/tils/suppose.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // suppose wraps any value, not only the core types, and the box returns the value it holds.
  auto text = tils::suppose(std::string("abc"));
  std::cout << "wrapped text: " << text.value() << '\n';
  std::cout << "wrapped length: " << text.value().size() << '\n';
  return 0;
}
