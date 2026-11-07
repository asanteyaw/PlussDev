#include <pluss/tils/auxiliary/rng_seed.h>

#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <thread>
#include <vector>

namespace {

// Allocate one derived seed per worker. Which worker gets which seed depends on
// scheduling, so the program compares the sorted set, not the assignment.
std::vector<std::uint64_t> allocate_concurrently(std::size_t count) {
  std::vector<std::uint64_t> seeds(count);
  std::vector<std::thread> workers;
  workers.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    workers.emplace_back([&seeds, i] { seeds[i] = pluss::tils::next_rng_seed(); });
  }
  for (auto& worker : workers) worker.join();
  std::sort(seeds.begin(), seeds.end());
  return seeds;
}

} // namespace

int main() {
  std::cout << std::boolalpha;
  constexpr std::size_t workers = 32;

  pluss::tils::rng_seed(900);
  const auto first_run = allocate_concurrently(workers);
  std::cout << "allocated: " << first_run.size() << '\n';
  std::cout << "all distinct: "
            << (std::adjacent_find(first_run.begin(), first_run.end()) == first_run.end()) << '\n';

  // Reset to the same base and run again. The set of derived seeds is the same,
  // even though the thread that received each seed may differ.
  pluss::tils::rng_seed(900);
  const auto second_run = allocate_concurrently(workers);
  std::cout << "same set on replay: " << (first_run == second_run) << '\n';
}
