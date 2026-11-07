#include <pluss/tils/auxiliary/rng_seed.h>

#include <iostream>

int main() {
  // Setting the base also resets the stream position to zero.
  pluss::tils::rng_seed(12345);

  // Reading the base reports it; it neither advances nor resets the stream.
  std::cout << "base: " << pluss::tils::rng_seed() << '\n';
  std::cout << "base again: " << pluss::tils::rng_seed() << '\n';

  // Two derived seeds advance the stream. The base is unchanged afterwards.
  const auto first = pluss::tils::next_rng_seed();
  const auto second = pluss::tils::next_rng_seed();
  std::cout << "derived distinct: " << std::boolalpha << (first != second) << '\n';
  std::cout << "base after draws: " << pluss::tils::rng_seed() << '\n';
}
