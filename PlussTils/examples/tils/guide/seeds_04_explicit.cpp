#include <pluss/tils/aux/rng_seed.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;
  // A nonzero explicit seed is used as given. The stream does not move.
  pluss::tils::rng_seed(700);
  const auto expected_next = pluss::tils::next_rng_seed();

  pluss::tils::rng_seed(700);
  std::cout << "explicit seed kept: " << pluss::tils::seed_or_next(99) << '\n';
  std::cout << "stream untouched by explicit: "
            << (pluss::tils::next_rng_seed() == expected_next) << '\n';

  // Zero asks for the next derived seed, so it consumes one stream position.
  pluss::tils::rng_seed(800);
  const auto expected_zero = pluss::tils::next_rng_seed();
  pluss::tils::rng_seed(800);
  std::cout << "zero requests next: " << (pluss::tils::seed_or_next(0) == expected_zero) << '\n';
}
