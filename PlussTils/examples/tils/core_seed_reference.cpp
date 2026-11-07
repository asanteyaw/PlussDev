#include <pluss/tils/auxiliary/rng_seed.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

int main() {
  using pluss::tils::next_rng_seed;
  using pluss::tils::rng_seed;
  using pluss::tils::seed_or_next;

  rng_seed(12345);
  std::cout << "base: " << rng_seed() << '\n';
  const auto first = next_rng_seed();
  const auto second = next_rng_seed();
  std::cout << "first: " << first << '\n';
  std::cout << "second: " << second << '\n';
  std::cout << "base after draws: " << rng_seed() << '\n';

  rng_seed(12345);
  std::cout << "reset replays first: " << std::boolalpha
            << (next_rng_seed() == first) << '\n';
  std::cout << "reset replays second: "
            << (next_rng_seed() == second) << '\n';

  rng_seed(0);
  const auto zero_first = next_rng_seed();
  rng_seed(0);
  std::cout << "zero base: " << rng_seed() << '\n';
  std::cout << "zero base replays: "
            << (next_rng_seed() == zero_first) << '\n';

  rng_seed(12345);
  std::mt19937_64 left(next_rng_seed());
  std::mt19937_64 right(next_rng_seed());
  std::cout << "left differs from right: " << (left() != right()) << '\n';

  rng_seed(700);
  const auto expected_next = next_rng_seed();
  rng_seed(700);
  std::cout << "explicit seed: " << seed_or_next(99) << '\n';
  std::cout << "explicit preserves stream: "
            << (next_rng_seed() == expected_next) << '\n';

  rng_seed(800);
  const auto expected_zero = next_rng_seed();
  rng_seed(800);
  std::cout << "zero requests next: "
            << (seed_or_next(0) == expected_zero) << '\n';

  constexpr std::size_t count = 32;
  rng_seed(900);
  std::vector<std::uint64_t> concurrent(count);
  std::vector<std::thread> workers;
  workers.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    workers.emplace_back([&, i] { concurrent[i] = next_rng_seed(); });
  }
  for (auto& worker : workers) worker.join();
  std::sort(concurrent.begin(), concurrent.end());
  std::cout << "concurrent count: " << concurrent.size() << '\n';
  std::cout << "concurrent unique: "
            << (std::adjacent_find(concurrent.begin(), concurrent.end()) == concurrent.end())
            << '\n';
}
