#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // get returns a const reference to the payload. The reference is valid for as long as the let lives.
  tils::let names = pluss::make_note({"north", "south"});
  const auto& payload = names.get();
  std::cout << "payload count: " << payload.numel() << '\n';
  std::cout << "reference is const: " << std::is_const_v<std::remove_reference_t<decltype(payload)>> << '\n';
  return 0;
}
