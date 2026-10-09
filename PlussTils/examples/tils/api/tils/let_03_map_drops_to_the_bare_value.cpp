#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // map drops to the bare value when the function returns something that is not a payload.
  tils::let values = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3});
  auto count = values.map([](const pluss::Series& s) { return s.numel(); });
  std::cout << "map drops to the bare value: " << std::is_same_v<decltype(count), std::int64_t> << '\n';
  std::cout << "count: " << count << '\n';
  return 0;
}
