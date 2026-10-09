#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // unwrap on a binding that is about to end moves the payload out. The binding is then spent.
  tils::let names = pluss::make_note({"north", "south"});
  pluss::Note moved = std::move(names).unwrap();
  std::cout << "moved out count: " << moved.numel() << '\n';
  return 0;
}
