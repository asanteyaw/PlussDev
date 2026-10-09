#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // Only the core payload types are opted in, so a plain int is not a let payload.
  std::cout << "int is a let payload: " << tils::let_payload<int> << '\n';
  std::cout << "series is a let payload: " << tils::let_payload<pluss::Series> << '\n';
  return 0;
}
