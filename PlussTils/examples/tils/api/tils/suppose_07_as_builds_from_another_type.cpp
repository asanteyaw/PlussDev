#include <pluss/tils/suppose.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // suppose.as<To>(value) builds a box of To from a value of another type.
  auto from_text = tils::suppose.as<std::string>("literal");
  std::cout << "built string: " << from_text.value() << '\n';
  std::cout << "built value is a std::string: " << std::is_same_v<decltype(from_text.value()), std::string&> << '\n';
  return 0;
}
