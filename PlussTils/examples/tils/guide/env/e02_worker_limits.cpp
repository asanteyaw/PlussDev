// A worker pool reads its thread count and its request timeout. Operators can set absurd values by
// accident, so the readers clamp each value into a safe range instead of trusting it.
#include <pluss/tils/env.h>

#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace env = pluss::tils::env;

int main() {
  setenv("PLUSS_GUIDE_THREADS", "64", 1);
  setenv("PLUSS_GUIDE_TIMEOUT", "-2.5", 1);

  const int threads = env::get_int_clamped("PLUSS_GUIDE_THREADS", 4, 1, 16);
  const double timeout = env::get_double_clamped("PLUSS_GUIDE_TIMEOUT", 30.0, 0.5, 120.0);
  std::cout << "threads: " << threads << '\n';
  std::cout << "timeout seconds: " << timeout << '\n';

  // A variable that is not set takes the fallback, and the clamp applies to the fallback as well.
  unsetenv("PLUSS_GUIDE_RETRIES");
  std::cout << "retries when unset: " << env::get_int_clamped("PLUSS_GUIDE_RETRIES", 5, 0, 3) << '\n';

  // Bounds given in the wrong order are swapped, so the range is still the one between them.
  std::cout << "timeout with reversed bounds: " << env::get_double_clamped("PLUSS_GUIDE_TIMEOUT", 30.0, 120.0, 0.5) << '\n';

  // A bound that is not finite is a programming error, and the reader refuses it at the call.
  try {
    (void)env::get_double_clamped("PLUSS_GUIDE_TIMEOUT", 30.0, 0.5, std::numeric_limits<double>::infinity());
  } catch (const std::invalid_argument&) {
    std::cout << "an infinite bound is refused at the call\n";
  }
  return 0;
}
