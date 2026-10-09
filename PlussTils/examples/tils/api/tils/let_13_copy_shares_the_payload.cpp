#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // A copy of a let gives a second binding that reads the same payload. Both names report the same count.
  tils::let first = pluss::make_note({"a", "b", "c"});
  tils::let second = first;
  std::cout << "first count: " << first.get().numel() << '\n';
  std::cout << "second count: " << second.get().numel() << '\n';
  return 0;
}
