// Turning a Series of numbers into a mask of true and false values. The mask marks the readings that
// passed a limit, and the conversion to boolean keeps every reading that is not zero as true.
#include <pluss/mic.h>
#include <pluss/tils/auxiliary/scalar_cast.h>

#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_platform(pluss::pCPU);
  const auto flags = pluss::make_series(std::vector<double>{0.0, 1.5, -2.0, 0.0}, {4}, cfg);
  const auto mask = pluss::tils::as_bool(flags, cfg);

  std::cout << "mask dtype is bool: " << (mask.dtype().scalar == pluss::ScalarType::Bool) << '\n';
  std::cout << "mask values:";
  for (bool b : mask.host<bool>()) std::cout << ' ' << b;
  std::cout << '\n';
  return 0;
}
