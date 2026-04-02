#pragma once
#include <atomic>
#include <cstdint>
#include <algorithm>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace pluss::msharp {

inline thread_local bool g_in_parallel_worker = false;

class ParallelRuntime final {
 public:
  ParallelRuntime() {
    const unsigned hw = std::max(1u, std::thread::hardware_concurrency());
    const unsigned nworkers = (hw > 1) ? (hw - 1) : 0;
    workers_.reserve(nworkers);
    for (unsigned i = 0; i < nworkers; ++i) {
      workers_.emplace_back([this] { worker_loop_(); });
    }
  }

  ~ParallelRuntime() {
    {
      std::lock_guard<std::mutex> lock(mu_);
      stop_ = true;
      ++generation_;
    }
    cv_start_.notify_all();
    for (auto& th : workers_) {
      if (th.joinable()) th.join();
    }
  }

  bool has_workers() const noexcept { return !workers_.empty(); }

  void run(std::int64_t begin, std::int64_t end, std::int64_t grain,
           const std::function<void(std::int64_t, std::int64_t)>& fn) {
    const std::int64_t n = end - begin;
    const std::int64_t num_blocks = (n + grain - 1) / grain;
    if (num_blocks <= 1 || workers_.empty()) {
      fn(begin, end);
      return;
    }

    {
      std::lock_guard<std::mutex> lock(mu_);
      begin_ = begin;
      end_ = end;
      grain_ = grain;
      num_blocks_ = num_blocks;
      next_block_.store(0, std::memory_order_release);
      remaining_.store(static_cast<std::int64_t>(workers_.size()) + 1, std::memory_order_release);
      fn_ = fn;
      ++generation_;
    }

    cv_start_.notify_all();

    run_blocks_();
    if (remaining_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      std::lock_guard<std::mutex> lock(mu_);
      cv_done_.notify_one();
    }

    std::unique_lock<std::mutex> lock(mu_);
    cv_done_.wait(lock, [&] { return remaining_.load(std::memory_order_acquire) == 0; });
    fn_ = nullptr;
  }

 private:
  void run_blocks_() {
    while (true) {
      const std::int64_t b = next_block_.fetch_add(1, std::memory_order_relaxed);
      if (b >= num_blocks_) break;
      const std::int64_t b0 = begin_ + b * grain_;
      const std::int64_t b1 = std::min(end_, b0 + grain_);
      fn_(b0, b1);
    }
  }

  void worker_loop_() {
    g_in_parallel_worker = true;
    std::uint64_t seen_generation = 0;
    std::unique_lock<std::mutex> lock(mu_);
    while (true) {
      cv_start_.wait(lock, [&] { return stop_ || generation_ != seen_generation; });
      if (stop_) break;
      seen_generation = generation_;
      lock.unlock();

      run_blocks_();
      if (remaining_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        std::lock_guard<std::mutex> done_lock(mu_);
        cv_done_.notify_one();
      }

      lock.lock();
    }
    g_in_parallel_worker = false;
  }

  std::mutex mu_;
  std::condition_variable cv_start_;
  std::condition_variable cv_done_;
  std::vector<std::thread> workers_;

  bool stop_ = false;
  std::uint64_t generation_ = 0;

  std::function<void(std::int64_t, std::int64_t)> fn_;
  std::int64_t begin_ = 0;
  std::int64_t end_ = 0;
  std::int64_t grain_ = 1;
  std::int64_t num_blocks_ = 0;
  std::atomic<std::int64_t> next_block_{0};
  std::atomic<std::int64_t> remaining_{0};
};

inline ParallelRuntime& runtime_() {
  static ParallelRuntime rt;
  return rt;
}

// Thread-parallel for CPU backed by a persistent worker pool.
inline void parallel_for(std::int64_t begin, std::int64_t end, std::int64_t grain,
                         const std::function<void(std::int64_t, std::int64_t)>& fn) {
  if (end <= begin) return;
  if (grain <= 0) grain = 1;
  const std::int64_t n = end - begin;
  if (n <= grain || g_in_parallel_worker) {
    fn(begin, end);
    return;
  }
  auto& rt = runtime_();
  if (!rt.has_workers()) {
    fn(begin, end);
    return;
  }
  rt.run(begin, end, grain, fn);
}

} // namespace pluss::msharp
