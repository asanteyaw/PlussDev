#include <pluss/tils/auxiliary/rng_seed.h>

#include <iostream>

int main() {
  // A first run of the program: reset, then draw three values.
  pluss::tils::rng_seed(12345);
  const auto run_one_a = pluss::tils::next_rng_seed();
  const auto run_one_b = pluss::tils::next_rng_seed();
  const auto run_one_c = pluss::tils::next_rng_seed();

  // A second run of the same program: reset to the same base, draw again.
  pluss::tils::rng_seed(12345);
  const auto run_two_a = pluss::tils::next_rng_seed();
  const auto run_two_b = pluss::tils::next_rng_seed();
  const auto run_two_c = pluss::tils::next_rng_seed();

  std::cout << "position one replays: " << std::boolalpha << (run_one_a == run_two_a) << '\n';
  std::cout << "position two replays: " << (run_one_b == run_two_b) << '\n';
  std::cout << "position three replays: " << (run_one_c == run_two_c) << '\n';

  // Resetting to a different base produces a different sequence.
  pluss::tils::rng_seed(54321);
  std::cout << "different base differs at position one: "
            << (pluss::tils::next_rng_seed() != run_one_a) << '\n';
}
