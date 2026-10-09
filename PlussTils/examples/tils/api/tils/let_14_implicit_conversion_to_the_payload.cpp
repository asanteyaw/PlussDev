#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // A let converts implicitly to a const reference of its payload, which is what allows passing it on.
  tils::let names = pluss::make_note({"north", "south"});
  const pluss::Note& view = names;
  std::cout << "viewed count: " << view.numel() << '\n';
  return 0;
}
