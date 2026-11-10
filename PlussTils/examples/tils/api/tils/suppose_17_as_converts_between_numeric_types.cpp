#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // as<To> builds a box of To from a value of another type, here an integer read as a double.
  auto ratio = tils::flow.as<double>(7);
  std::cout << "as double: " << ratio.value() << '\n';
  std::cout << "type is double: " << std::is_same_v<decltype(ratio.value()), double&> << '\n';
  return 0;
}
