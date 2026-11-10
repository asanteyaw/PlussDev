#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // A const box gives a const reference, so the value can be read but not changed through it.
  const auto fixed = tils::flow(std::string("read only"));
  std::cout << "const reference type: " << std::is_const_v<std::remove_reference_t<decltype(fixed.value())>> << '\n';
  std::cout << "read value: " << fixed.value() << '\n';
  return 0;
}
