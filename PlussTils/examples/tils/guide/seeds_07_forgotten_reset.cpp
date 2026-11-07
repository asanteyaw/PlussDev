#include <pluss/tils/kit/rng_seed.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;

  // Run one: reset, then draw.
  pluss::tils::rng_seed(12345);
  const auto run_one = pluss::tils::next_rng_seed();

  // Run two without a reset. The stream has already moved, so this run
  // draws a different value even though the code looks identical.
  const auto run_two_forgotten_reset = pluss::tils::next_rng_seed();
  std::cout << "forgotten reset matches run one: " << (run_two_forgotten_reset == run_one) << '\n';

  // Run two done correctly: reset first.
  pluss::tils::rng_seed(12345);
  const auto run_two = pluss::tils::next_rng_seed();
  std::cout << "reset before run two matches run one: " << (run_two == run_one) << '\n';
}
