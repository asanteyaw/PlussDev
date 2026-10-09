#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // The binding itself cannot change, so assignment does not compile. This check reads the trait, which says so.
  std::cout << "let is copy-assignable: " << std::is_copy_assignable_v<tils::let<pluss::Series>> << '\n';
  std::cout << "let is move-assignable: " << std::is_move_assignable_v<tils::let<pluss::Series>> << '\n';
  return 0;
}
