#include <pluss/pluss.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <span>
#include <string>
#include <vector>

#include "CodeN/core/AlignedAllocator.h"

namespace {

using clock_type = std::chrono::steady_clock;

struct BenchConfig {
  std::int64_t n = 200000;
  std::int64_t rounds = 8;
};

pluss::Note make_note_1d(const std::vector<std::string>& values) {
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(values.size())};
  return pluss::make_note(values, std::span<const std::int64_t>(shape.data(), shape.size()));
}

pluss::Note make_note_2d_prefix_cols(const std::vector<std::string>& values) {
  constexpr std::int64_t cols = 256;
  const std::int64_t usable = static_cast<std::int64_t>(values.size()) / cols * cols;
  std::vector<std::string> flat;
  flat.reserve(static_cast<std::size_t>(usable));
  for (std::int64_t i = 0; i < usable; ++i) {
    flat.push_back(values[static_cast<std::size_t>(i)]);
  }
  const std::array<std::int64_t, 2> shape{usable / cols, cols};
  return pluss::make_note(flat, std::span<const std::int64_t>(shape.data(), shape.size()));
}

std::vector<std::string> make_regression_corpus(std::int64_t n) {
  const std::vector<std::string> vocab{
      "alpha",
      "beta",
      "gamma",
      "",
      "delta",
      "東京",
      "naïve",
      "ßeta",
      "εpsilon",
      "z"};

  std::vector<std::string> out;
  out.reserve(static_cast<std::size_t>(std::max<std::int64_t>(0, n)));
  for (std::int64_t i = 0; i < n; ++i) {
    const auto idx = static_cast<std::size_t>((i * 1315423911ULL + 2654435761ULL) % vocab.size());
    out.push_back(vocab[idx]);
  }
  return out;
}

template <class Fn>
double run_bench_ms(Fn&& fn, std::int64_t rounds) {
  // Warmup.
  (void)fn();
  const auto t0 = clock_type::now();
  for (std::int64_t r = 0; r < rounds; ++r) {
    (void)fn();
  }
  const auto t1 = clock_type::now();
  const auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
  return static_cast<double>(us) / (1000.0 * static_cast<double>(rounds));
}

struct LaneBenchResult {
  double milliseconds{0.0};
  double million_lanes_per_second{0.0};
  std::uint64_t allocations{0};
  std::uint64_t bytes{0};
};

template <class Fn>
LaneBenchResult run_lane_bench(Fn&& fn, std::int64_t lanes,
                               std::int64_t rounds) {
  (void)fn();
  pluss::coden::detail::LaneAllocationScope tracking;
  const auto t0 = clock_type::now();
  for (std::int64_t round = 0; round < rounds; ++round) (void)fn();
  const auto t1 = clock_type::now();
  const auto allocations = tracking.snapshot();
  const auto elapsed =
      std::chrono::duration<double, std::milli>(t1 - t0).count();
  const auto total_lanes = static_cast<double>(lanes) * rounds;
  return {
      elapsed / static_cast<double>(rounds),
      elapsed > 0.0 ? total_lanes / (elapsed * 1000.0) : 0.0,
      allocations.allocations,
      allocations.bytes,
  };
}

void print_lane_bench(std::string_view name, const LaneBenchResult& result,
                      std::int64_t rounds) {
  std::cout << name << "_ms=" << result.milliseconds << "\n";
  std::cout << name << "_million_lanes_per_second="
            << result.million_lanes_per_second << "\n";
  std::cout << name << "_allocations_per_round="
            << static_cast<double>(result.allocations) / rounds << "\n";
  std::cout << name << "_bytes_per_round="
            << static_cast<double>(result.bytes) / rounds << "\n";
}

} // namespace

int main(int argc, char** argv) {
  BenchConfig cfg{};
  if (argc > 1) cfg.n = std::max<std::int64_t>(1000, std::stoll(argv[1]));
  if (argc > 2) cfg.rounds = std::max<std::int64_t>(1, std::stoll(argv[2]));

  const auto corpus = make_regression_corpus(cfg.n);
  const auto note = make_note_1d(corpus);
  const auto small_corpus = make_regression_corpus(32);
  const auto small_note = make_note_1d(small_corpus);
  const auto note2d = make_note_2d_prefix_cols(corpus);
  std::string keyword_doc;
  keyword_doc.reserve(static_cast<std::size_t>(cfg.n * 8));
  for (std::int64_t i = 0; i < cfg.n; ++i) {
    if (!keyword_doc.empty()) keyword_doc.push_back(' ');
    keyword_doc.append(corpus[static_cast<std::size_t>(i)]);
  }
  const auto keyword_note = pluss::make_note(keyword_doc);
  std::vector<std::int64_t> gather_idx(static_cast<std::size_t>(cfg.n));
  std::iota(gather_idx.begin(), gather_idx.end(), 0);
  std::reverse(gather_idx.begin(), gather_idx.end());

  volatile std::int64_t sink = 0;

  const double sort_ms = run_bench_ms(
      [&]() {
        auto y = note.sort();
        sink += y.numel();
        return y.numel();
      },
      cfg.rounds);

  const double sort_2d_ms = run_bench_ms(
      [&]() {
        auto y = note2d.sort();
        sink += y.numel();
        return y.numel();
      },
      cfg.rounds);

  const double unique_ms = run_bench_ms(
      [&]() {
        auto y = note.unique();
        sink += y.numel();
        return y.numel();
      },
      cfg.rounds);

  const double dedup_ms = run_bench_ms(
      [&]() {
        auto y = note.deduplicate();
        sink += y.numel();
        return y.numel();
      },
      cfg.rounds);

  const double take_ms = run_bench_ms(
      [&]() {
        auto y = note.take(std::span<const std::int64_t>(gather_idx.data(), gather_idx.size()));
        sink += y.numel();
        return y.numel();
      },
      cfg.rounds);

  const double keywords_ms = run_bench_ms(
      [&]() {
        auto y = keyword_note.nlp().keywords(10);
        sink += y.tokens().numel();
        return y.tokens().numel();
      },
      cfg.rounds);

  const auto small_bool = run_lane_bench(
      [&]() { return small_note.contains("a"); }, small_note.numel(),
      cfg.rounds);
  const auto large_bool = run_lane_bench(
      [&]() { return note.contains("a"); }, note.numel(), cfg.rounds);
  const auto small_bool_left = small_note.contains("a");
  const auto small_bool_right = small_note.contains("e");
  const auto large_bool_left = note.contains("a");
  const auto large_bool_right = note.contains("e");
  const auto small_bool_or = run_lane_bench(
      [&]() { return small_bool_left | small_bool_right; },
      small_note.numel(), cfg.rounds);
  const auto large_bool_or = run_lane_bench(
      [&]() { return large_bool_left | large_bool_right; }, note.numel(),
      cfg.rounds);
  const auto small_int = run_lane_bench(
      [&]() { return small_note.lengths_bytes(); }, small_note.numel(),
      cfg.rounds);
  const auto large_int = run_lane_bench(
      [&]() { return note.lengths_bytes(); }, note.numel(), cfg.rounds);

  std::cout << "CodeN CPU Perf Bench\n";
  std::cout << "n=" << cfg.n << ", rounds=" << cfg.rounds << "\n";
  std::cout << "sort_ms=" << sort_ms << "\n";
  std::cout << "sort_2d_ms=" << sort_2d_ms << "\n";
  std::cout << "unique_ms=" << unique_ms << "\n";
  std::cout << "deduplicate_ms=" << dedup_ms << "\n";
  std::cout << "take_ms=" << take_ms << "\n";
  std::cout << "nlp_keywords_ms=" << keywords_ms << "\n";
  print_lane_bench("contains_small", small_bool, cfg.rounds);
  print_lane_bench("contains_large", large_bool, cfg.rounds);
  print_lane_bench("bool_or_small", small_bool_or, cfg.rounds);
  print_lane_bench("bool_or_large", large_bool_or, cfg.rounds);
  print_lane_bench("lengths_bytes_small", small_int, cfg.rounds);
  print_lane_bench("lengths_bytes_large", large_int, cfg.rounds);
  std::cout << "sink=" << sink << "\n";
  return 0;
}
