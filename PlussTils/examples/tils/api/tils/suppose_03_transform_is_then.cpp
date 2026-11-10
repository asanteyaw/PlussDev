#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // transform is the same operation under a second name, so either spelling gives the same box.
  auto base = tils::flow(std::string("ab"));
  auto via_then = base.then([](const std::string& s) { return s + "cd"; });
  auto via_transform = base.transform([](const std::string& s) { return s + "cd"; });
  std::cout << "then: " << via_then.value() << '\n';
  std::cout << "transform: " << via_transform.value() << '\n';
  return 0;
}
