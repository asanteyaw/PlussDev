#include <pluss/tils/kit/rng_seed.h>

#include <iostream>

int main() {
  std::cout << std::boolalpha;
  // Zero is a legitimate base. It is not a request for an implicit seed.
  pluss::tils::rng_seed(0);
  std::cout << "base is zero: " << (pluss::tils::rng_seed() == 0) << '\n';

  // The base still produces a stream of derived seeds, and that stream replays.
  pluss::tils::rng_seed(0);
  const auto first = pluss::tils::next_rng_seed();
  pluss::tils::rng_seed(0);
  std::cout << "zero base replays: " << (pluss::tils::next_rng_seed() == first) << '\n';
  std::cout << "zero base first derived is nonzero: " << (first != 0) << '\n';
}
