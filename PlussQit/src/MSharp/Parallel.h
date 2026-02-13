#pragma once
#include <cstdint>
#include <thread>
#include <vector>
#include <algorithm>
#include <functional>

namespace pluss::msharp {

// Simple static thread-parallel for CPU.
// Uses a fixed number of threads = hardware_concurrency (>=1).
inline void parallel_for(std::int64_t begin, std::int64_t end, std::int64_t grain,
                         const std::function<void(std::int64_t, std::int64_t)>& fn) {
  if (end <= begin) return;
  const std::int64_t n = end - begin;
  if (grain <= 0) grain = 1;
  const unsigned hw = std::max(1u, std::thread::hardware_concurrency());
  // If small, run single-threaded
  if (n <= grain || hw == 1) {
    fn(begin, end);
    return;
  }

  const std::int64_t num_blocks = (n + grain - 1) / grain;
  const std::int64_t num_threads = std::min<std::int64_t>(num_blocks, (std::int64_t)hw);

  std::vector<std::thread> threads;
  threads.reserve((std::size_t)num_threads);

  // simple block scheduling
  for (std::int64_t t = 0; t < num_threads; ++t) {
    threads.emplace_back([=, &fn]{
      for (std::int64_t b = t; b < num_blocks; b += num_threads) {
        const std::int64_t b0 = begin + b * grain;
        const std::int64_t b1 = std::min(end, b0 + grain);
        fn(b0, b1);
      }
    });
  }
  for (auto& th : threads) th.join();
}

} // namespace pluss::msharp
