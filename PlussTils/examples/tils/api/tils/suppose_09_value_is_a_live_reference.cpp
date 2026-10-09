#include <pluss/tils/suppose.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // value() on an lvalue box returns a reference to the held value, so a change through it is kept.
  auto counter = tils::suppose(10);
  counter.value() += 5;
  std::cout << "held value after an edit: " << counter.value() << '\n';
  return 0;
}
