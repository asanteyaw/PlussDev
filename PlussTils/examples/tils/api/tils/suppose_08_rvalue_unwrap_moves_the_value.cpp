#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // Calling unwrap on a box that is about to end moves the value out, so no copy is made.
  auto items = tils::flow(std::vector<int>{4, 5, 6});
  std::vector<int> taken = std::move(items).unwrap();
  std::cout << "taken size: " << taken.size() << '\n';
  return 0;
}
