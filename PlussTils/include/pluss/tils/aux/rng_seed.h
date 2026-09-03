#pragma once

#include <atomic>
#include <cstdint>

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

inline std::atomic<std::uint64_t>& rng_seed_state_() noexcept {
  static std::atomic<std::uint64_t> state{kDefaultSeed};
  return state;
}

inline std::atomic<std::uint64_t>& rng_seed_stream_counter_() noexcept {
  static std::atomic<std::uint64_t> counter{0};
  return counter;
}

} // namespace detail

inline void rng_seed(std::uint64_t seed) noexcept {
  detail::rng_seed_state_().store(seed, std::memory_order_relaxed);
  detail::rng_seed_stream_counter_().store(0, std::memory_order_relaxed);
}

inline std::uint64_t rng_seed() noexcept {
  return detail::rng_seed_state_().load(std::memory_order_relaxed);
}

inline std::uint64_t next_rng_seed() noexcept {
  const std::uint64_t base = rng_seed();
  const std::uint64_t offset =
      detail::rng_seed_stream_counter_().fetch_add(1, std::memory_order_relaxed) + 1ULL;
  return detail::splitmix64_(base + offset);
}

inline std::uint64_t seed_or_next(std::uint64_t seed) noexcept {
  return seed == 0 ? next_rng_seed() : seed;
}

} // namespace pluss::tils
