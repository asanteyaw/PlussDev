#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // let binds a core payload. The binding is read-only, and reading the payload gives the same numbers.
  tils::let values = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3});
  std::cout << "bound count: " << values.get().numel() << '\n';
  return 0;
}
