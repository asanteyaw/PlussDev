#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // The result type follows the function. A string can become a length, a number, or anything else.
  auto text = tils::flow(std::string("hello"));
  auto length = text.then([](const std::string& s) { return s.size(); });
  std::cout << "length type is size_t: " << std::is_same_v<decltype(length.value()), std::size_t&> << '\n';
  std::cout << "length: " << length.value() << '\n';
  return 0;
}
