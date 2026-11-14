#pragma once
// INTERNAL: a minimal fork-join loop for the pricing engines (the same pattern as the fused COS kernel in cos_pricing.cpp).
#include <algorithm>
#include <cstddef>
#include <thread>
#include <vector>

namespace pluss::subsp::math::finance::internal {

// Runs fn(begin, end) over disjoint chunks covering [0, count).  `work` is the approximate cost of the whole loop in flops: below `threshold` it
// runs inline, so small problems pay no thread start-up.  fn must be safe to call concurrently on disjoint index ranges.
template <typename Fn>
void parallel_for(std::size_t count, double work, Fn&& fn, double threshold = 2e6) {
  const unsigned hardware = std::max(1u, std::thread::hardware_concurrency());
  const std::size_t threads = std::min<std::size_t>({count, hardware, work < threshold ? 1u : 64u});
  if (threads <= 1) {
    if (count > 0) fn(std::size_t{0}, count);
    return;
  }
  std::vector<std::thread> pool;
  pool.reserve(threads);
  const std::size_t chunk = (count + threads - 1) / threads;
  for (std::size_t t = 0; t < threads; ++t) {
    const std::size_t begin = t * chunk, end = std::min(count, begin + chunk);
    if (begin >= end) break;
    pool.emplace_back([&fn, begin, end] { fn(begin, end); });
  }
  for (auto& th : pool) th.join();
}

}  // namespace pluss::subsp::math::finance::internal
