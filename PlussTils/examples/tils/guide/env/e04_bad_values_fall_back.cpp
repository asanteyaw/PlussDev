// Operators make typing mistakes, such as a unit after a number or a word where a flag belongs. The
// typed readers never crash on those. They fall back to the value the program gave them.
#include <pluss/tils/env.h>

#include <cstdlib>
#include <iostream>

namespace env = pluss::tils::env;

int main() {
  setenv("PLUSS_GUIDE_CACHE_MB", "256", 1);
  std::cout << "valid integer: " << env::get_int("PLUSS_GUIDE_CACHE_MB", 64) << '\n';

  setenv("PLUSS_GUIDE_CACHE_MB", "256mb", 1);
  std::cout << "integer with a unit: " << env::get_int("PLUSS_GUIDE_CACHE_MB", 64) << '\n';

  setenv("PLUSS_GUIDE_VERBOSE", "perhaps", 1);
  std::cout << "unknown flag word: " << std::boolalpha << env::get_bool("PLUSS_GUIDE_VERBOSE", false) << '\n';

  setenv("PLUSS_GUIDE_RATE", "nan", 1);
  std::cout << "non-finite double: " << env::get_double("PLUSS_GUIDE_RATE", 0.25) << '\n';

  setenv("PLUSS_GUIDE_RATE", "  0.75  ", 1);
  std::cout << "padded double: " << env::get_double("PLUSS_GUIDE_RATE", 0.25) << '\n';
  return 0;
}
