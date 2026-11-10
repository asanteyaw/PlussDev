#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // unwrap on an lvalue box returns a reference, like value(), so the box keeps owning the value.
  auto items = tils::flow(std::vector<int>{1, 2});
  items.unwrap().push_back(3);
  std::cout << "size after push through unwrap: " << items.value().size() << '\n';
  return 0;
}
