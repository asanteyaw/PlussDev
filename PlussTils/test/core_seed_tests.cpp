#include <pluss/tils/aux/rng_seed.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

namespace {

void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "core seed test failed: " << message << '\n';
    std::exit(1);
  }
}

} // namespace

int main() {
  using pluss::tils::next_rng_seed;
  using pluss::tils::rng_seed;
  using pluss::tils::seed_or_next;

  rng_seed(12345);
  require(rng_seed() == 12345, "getter must return the configured base seed");
  const auto first = next_rng_seed();
  const auto second = next_rng_seed();
  require(first != second, "successive stream seeds must differ");

  rng_seed(12345);
  require(next_rng_seed() == first, "reset must replay the first stream seed");
  require(next_rng_seed() == second, "reset must replay the complete stream order");

  rng_seed(700);
  const auto expected_next = next_rng_seed();
  rng_seed(700);
  require(seed_or_next(99) == 99, "a nonzero explicit seed must be returned unchanged");
  require(next_rng_seed() == expected_next, "an explicit seed must not consume the shared stream");

  rng_seed(800);
  const auto expected_zero = next_rng_seed();
  rng_seed(800);
  require(seed_or_next(0) == expected_zero, "zero must delegate to the next shared seed");

  constexpr std::size_t count = 64;
  rng_seed(900);
  std::vector<std::uint64_t> concurrent(count);
  std::vector<std::thread> workers;
  workers.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    workers.emplace_back([&, i] { concurrent[i] = next_rng_seed(); });
  }
  for (auto& worker : workers) worker.join();
  std::sort(concurrent.begin(), concurrent.end());
  require(std::adjacent_find(concurrent.begin(), concurrent.end()) == concurrent.end(),
          "concurrent stream allocation must not duplicate a counter position");

  std::cout << "core seed tests passed\n";
}
