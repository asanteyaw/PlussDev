#include <pluss/tils/suppose.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // tap also works on a const box, because it only reads the held value for its side effect.
  const auto words = tils::suppose(std::vector<std::string>{"a", "bb"});
  words.tap([](const std::vector<std::string>& v) { std::cout << "const tap count: " << v.size() << '\n'; });
  std::cout << "const box after tap: " << words.value().size() << '\n';
  return 0;
}
