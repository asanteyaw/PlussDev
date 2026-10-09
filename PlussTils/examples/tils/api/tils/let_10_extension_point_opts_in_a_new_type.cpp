#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

struct Reading {
  double value;
};

// Any type can join the family by specializing enable_let. Nothing else is needed.
template <>
inline constexpr bool pluss::tils::enable_let<Reading> = true;

int main() {
  tils::let reading = Reading{2.5};
  std::cout << "custom payload value: " << reading.get().value << '\n';
  std::cout << "custom payload is a let payload: " << tils::let_payload<Reading> << '\n';
  return 0;
}
