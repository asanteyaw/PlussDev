#include <pluss/pluss.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <span>
#include <string>
#include <vector>

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

} // namespace

int main(int argc, char** argv) {
  BenchConfig cfg{};
  if (argc > 1) cfg.n = std::max<std::int64_t>(1000, std::stoll(argv[1]));
  if (argc > 2) cfg.rounds = std::max<std::int64_t>(1, std::stoll(argv[2]));

  const auto corpus = make_regression_corpus(cfg.n);
  const auto note = make_note_1d(corpus);
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
        auto y = keyword_note.nlp_keywords(10);
        sink += y.tokens().numel();
        return y.tokens().numel();
      },
      cfg.rounds);

  std::cout << "CodeN CPU Perf Bench\n";
  std::cout << "n=" << cfg.n << ", rounds=" << cfg.rounds << "\n";
  std::cout << "sort_ms=" << sort_ms << "\n";
  std::cout << "sort_2d_ms=" << sort_2d_ms << "\n";
  std::cout << "unique_ms=" << unique_ms << "\n";
  std::cout << "deduplicate_ms=" << dedup_ms << "\n";
  std::cout << "take_ms=" << take_ms << "\n";
  std::cout << "nlp_keywords_ms=" << keywords_ms << "\n";
  std::cout << "sink=" << sink << "\n";
  return 0;
}
