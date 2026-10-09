#include <pluss/tils/suppose.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // then works on a const box too. The function receives the held value as a const reference.
  const auto word = tils::suppose(std::string("data"));
  auto upper_size = word.then([](const std::string& s) { return s.size() * 2; });
  std::cout << "const box then result: " << upper_size.value() << '\n';
  std::cout << "const box still holds: " << word.value() << '\n';
  return 0;
}
