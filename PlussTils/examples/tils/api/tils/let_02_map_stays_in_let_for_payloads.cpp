#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // map keeps the binding when the function returns a payload type.
  tils::let values = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3});
  auto doubled = values.map([](const pluss::Series& s) { return s * 2.0; });
  std::cout << "map returns a let when the result is a payload: " << std::is_same_v<decltype(doubled), tils::let<pluss::Series>> << '\n';
  return 0;
}
