#pragma once

#include <cstdint>
#include <mutex>

namespace pluss::tils {
namespace detail {

inline constexpr std::uint64_t kDefaultSeed = 0x4d595df4d0f33173ULL;

inline std::uint64_t splitmix64_(std::uint64_t x) noexcept {
  x += 0x9e3779b97f4a7c15ULL;
  std::uint64_t z = x;
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
  return z ^ (z >> 31);
}

struct rng_seed_state {
  std::mutex mutex{};
  std::uint64_t base{kDefaultSeed};
  std::uint64_t counter{0};
};

inline rng_seed_state& rng_seed_state_() noexcept {
  static rng_seed_state state{};
  return state;
}

} // namespace detail

inline void rng_seed(std::uint64_t seed) {
  auto& state = detail::rng_seed_state_();
  const std::scoped_lock lock(state.mutex);
  state.base = seed;
  state.counter = 0;
}

inline std::uint64_t rng_seed() {
  auto& state = detail::rng_seed_state_();
  const std::scoped_lock lock(state.mutex);
  return state.base;
}

inline std::uint64_t next_rng_seed() {
  std::uint64_t base;
  std::uint64_t offset;
  {
    auto& state = detail::rng_seed_state_();
    const std::scoped_lock lock(state.mutex);
    base = state.base;
    offset = ++state.counter;
  }
  return detail::splitmix64_(base + offset);
}

inline std::uint64_t seed_or_next(std::uint64_t seed) {
  return seed == 0 ? next_rng_seed() : seed;
}

} // namespace pluss::tils
