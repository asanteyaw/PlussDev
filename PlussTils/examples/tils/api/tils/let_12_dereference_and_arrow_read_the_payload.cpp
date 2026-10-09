#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // * and -> read the same payload that get returns, so either spelling reads the same numbers.
  tils::let values = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4});
  std::cout << "through *: " << (*values).numel() << '\n';
  std::cout << "through ->: " << values->numel() << '\n';
  return 0;
}
