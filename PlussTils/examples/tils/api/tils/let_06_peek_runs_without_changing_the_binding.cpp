#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // peek runs a function on the payload and returns the same binding, so it can be chained.
  tils::let names = pluss::make_note({"north", "south", "east"});
  names.peek([](const pluss::Note& n) { std::cout << "peeked count: " << n.numel() << '\n'; })
       .peek([](const pluss::Note& n) { std::cout << "peeked again: " << n.numel() << '\n'; });
  std::cout << "binding unchanged: " << names.get().numel() << '\n';
  return 0;
}
