#include <pluss/tils/kit/rng_seed.h>

#include <cstdint>
#include <iostream>
#include <random>

int main() {
  std::cout << std::boolalpha;

  // Build two engines from two consecutive derived seeds, and record their
  // first outputs.
  pluss::tils::rng_seed(12345);
  std::mt19937_64 left(pluss::tils::next_rng_seed());
  std::mt19937_64 right(pluss::tils::next_rng_seed());
  const std::uint64_t left_first = left();
  const std::uint64_t right_first = right();
  std::cout << "engines start differently: " << (left_first != right_first) << '\n';

  // Reset the stream and build the same pair again. Each engine replays
  // its own first output.
  pluss::tils::rng_seed(12345);
  std::mt19937_64 left_again(pluss::tils::next_rng_seed());
  std::mt19937_64 right_again(pluss::tils::next_rng_seed());
  std::cout << "left replays: " << (left_again() == left_first) << '\n';
  std::cout << "right replays: " << (right_again() == right_first) << '\n';

  // Advancing one engine does not move the other. Advance left_again by a
  // thousand draws, then compare the next outputs of the two right engines.
  for (int i = 0; i < 1000; ++i) (void)left_again();
  std::cout << "right unaffected by left: " << (right() == right_again()) << '\n';
}
