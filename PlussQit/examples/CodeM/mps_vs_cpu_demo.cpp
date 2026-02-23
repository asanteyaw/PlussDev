#include "pluss/codem/codem.h"
#include "pluss/codem/series_factory.h"
#include <pluss/pluss.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct BenchResult {
  double millis{0.0};
  float checksum{0.0f};
  pluss::Series last{pluss::Series::undefined()};
};

struct ErrorStats {
  double mean_abs{0.0};
  double max_abs{0.0};
  double mean_rel{0.0};
  double max_rel{0.0};
  double p95_rel{0.0};
  double p99_rel{0.0};
  std::size_t max_abs_idx{0};
  std::size_t max_rel_idx{0};
  double cpu_at_max_abs{0.0};
  double mps_at_max_abs{0.0};
  double cpu_at_max_rel{0.0};
  double mps_at_max_rel{0.0};
};

pluss::Series complex_chain(pluss::Series x,
                            const pluss::Series& w1,
                            const pluss::Series& w2,
                            int inner_iters) {
  const auto p = x.platform();
  const auto dt = pluss::ScalarType::Float32;

  const auto c_add = pluss::value(0.05, dt).to(p);
  const auto c_mul = pluss::value(0.75, dt).to(p);
  const auto c_div = pluss::value(1.25, dt).to(p);
  const auto c_eps = pluss::value(1e-3, dt).to(p);
  const auto c_shift = pluss::value(0.15, dt).to(p);

  for (int i = 0; i < inner_iters; ++i) {
    x = x.matmul(w1);
    x = x.mul(c_mul).add(c_add);
    x = x.abs().add(c_eps);
    x = x.log().exp();
    x = x.log1p().expm1();
    x = x.matmul(w2);
    x = x.abs().add(c_shift).sqrt();
    x = x.mul(c_div).div(c_mul).add(c_add);
  }
  return x;
}

BenchResult run_bench(pluss::Platform p, std::int64_t n, int inner_iters, int rounds, std::uint64_t seed) {
  pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(p);
  const auto x0 = pluss::randn({n, n}, cfg, seed);
  const auto w1 = pluss::randn({n, n}, cfg, seed + 1);
  const auto w2 = pluss::randn({n, n}, cfg, seed + 2);

  // Warm-up to reduce one-time setup noise.
  (void)complex_chain(x0, w1, w2, std::max(1, inner_iters / 2));

  BenchResult out;
  const auto t0 = std::chrono::steady_clock::now();
  for (int r = 0; r < rounds; ++r) {
    out.last = complex_chain(x0, w1, w2, inner_iters);
    out.checksum += out.last.to_vec<float>(0);
  }
  const auto t1 = std::chrono::steady_clock::now();
  out.millis = std::chrono::duration<double, std::milli>(t1 - t0).count();
  return out;
}

std::int64_t parse_i64(const char* s, const char* name) {
  if (s == nullptr) throw std::runtime_error(std::string("missing arg: ") + name);
  char* end = nullptr;
  const long long v = std::strtoll(s, &end, 10);
  if (end == s || (end != nullptr && *end != '\0')) {
    throw std::runtime_error(std::string("invalid integer for ") + name + ": " + s);
  }
  return static_cast<std::int64_t>(v);
}

int parse_i32(const char* s, const char* name) {
  const auto v = parse_i64(s, name);
  if (v < 1 || v > 1'000'000) {
    throw std::runtime_error(std::string("out of range for ") + name);
  }
  return static_cast<int>(v);
}

ErrorStats compare_stats(const pluss::Series& cpu_last, const pluss::Series& mps_last) {
  const auto c = cpu_last.to_host<float>();
  const auto g = mps_last.to_host<float>();
  if (c.data.size() != g.data.size()) {
    throw std::runtime_error("compare_stats: shape mismatch");
  }

  ErrorStats stats;
  if (c.data.empty()) {
    return stats;
  }

  constexpr double kEps = 1e-12;
  double abs_sum = 0.0;
  double rel_sum = 0.0;
  std::vector<double> rel_vals;
  rel_vals.reserve(c.data.size());
  for (std::size_t i = 0; i < c.data.size(); ++i) {
    const double a = static_cast<double>(c.data[i]);
    const double b = static_cast<double>(g.data[i]);
    const double abs_e = std::abs(a - b);
    // Symmetric relative error avoids exploding ratios when CPU value is ~0.
    const double rel_e = abs_e / std::max(std::max(std::abs(a), std::abs(b)), kEps);
    abs_sum += abs_e;
    rel_sum += rel_e;
    rel_vals.push_back(rel_e);
    if (abs_e > stats.max_abs) {
      stats.max_abs = abs_e;
      stats.max_abs_idx = i;
      stats.cpu_at_max_abs = a;
      stats.mps_at_max_abs = b;
    }
    if (rel_e > stats.max_rel) {
      stats.max_rel = rel_e;
      stats.max_rel_idx = i;
      stats.cpu_at_max_rel = a;
      stats.mps_at_max_rel = b;
    }
  }
  stats.mean_abs = abs_sum / static_cast<double>(c.data.size());
  stats.mean_rel = rel_sum / static_cast<double>(c.data.size());
  if (!rel_vals.empty()) {
    auto rel95 = rel_vals;
    auto rel99 = rel_vals;
    const std::size_t i95 = (rel95.size() * 95) / 100;
    const std::size_t i99 = (rel99.size() * 99) / 100;
    std::nth_element(rel95.begin(), rel95.begin() + static_cast<std::ptrdiff_t>(i95), rel95.end());
    std::nth_element(rel99.begin(), rel99.begin() + static_cast<std::ptrdiff_t>(i99), rel99.end());
    stats.p95_rel = rel95[i95];
    stats.p99_rel = rel99[i99];
  }
  return stats;
}

} // namespace

int main(int argc, char** argv) {
  try {
    std::int64_t n = 512;
    std::int64_t m = 512;
    int inner_iters = 3;
    int rounds = 3;

    // Supports both:
    // 1) [n inner_iters rounds]
    // 2) [n m inner_iters rounds] (legacy style; currently requires n == m)
    if (argc == 5) {
      n = parse_i64(argv[1], "n");
      m = parse_i64(argv[2], "m");
      inner_iters = parse_i32(argv[3], "inner_iters");
      rounds = parse_i32(argv[4], "rounds");
    } else {
      if (argc > 1) n = parse_i64(argv[1], "n");
      m = n;
      if (argc > 2) inner_iters = parse_i32(argv[2], "inner_iters");
      if (argc > 3) rounds = parse_i32(argv[3], "rounds");
    }

    if (n <= 0 || m <= 0) {
      throw std::runtime_error("shape sizes must be positive");
    }
    if (n != m) {
      throw std::runtime_error("mps_vs_cpu_demo currently benchmarks square shapes only (n == m)");
    }

    std::cout << "Benchmark settings\n";
    std::cout << "shape=(" << n << ", " << m << "), inner_iters=" << inner_iters
              << ", rounds=" << rounds << "\n";
    std::cout << "MPS usable: " << (pluss::mps::is_usable() ? "true" : "false") << "\n\n";

    const std::uint64_t seed = 42;
    const auto cpu = run_bench(pluss::pCPU, n, inner_iters, rounds, seed);
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "[CPU] total_ms=" << cpu.millis
              << ", avg_ms_per_round=" << (cpu.millis / static_cast<double>(rounds))
              << ", checksum=" << cpu.checksum << "\n";

    if (!pluss::mps::is_usable()) {
      std::cout << "\nMPS benchmark skipped because backend is not usable in this runtime/build.\n";
      return 0;
    }

    const auto mps = run_bench(pluss::pMPS, n, inner_iters, rounds, seed);
    std::cout << "[MPS] total_ms=" << mps.millis
              << ", avg_ms_per_round=" << (mps.millis / static_cast<double>(rounds))
              << ", checksum=" << mps.checksum << "\n";

    const auto cpu_last = cpu.last.to(pluss::pCPU, pluss::ScalarType::Float32);
    const auto mps_last = mps.last.to(pluss::pCPU, pluss::ScalarType::Float32);
    const auto err = compare_stats(cpu_last, mps_last);

    std::cout << "\nComparison\n";
    std::cout << "speed_ratio_cpu_over_mps=" << (cpu.millis / mps.millis) << "\n";
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "mean_abs_err=" << err.mean_abs << ", max_abs_err=" << err.max_abs << "\n";
    std::cout << "mean_rel_err(sym)=" << err.mean_rel
              << ", p95_rel_err(sym)=" << err.p95_rel
              << ", p99_rel_err(sym)=" << err.p99_rel
              << ", max_rel_err(sym)=" << err.max_rel << "\n";
    std::cout << "max_abs_at[idx=" << err.max_abs_idx << "] cpu=" << err.cpu_at_max_abs
              << ", mps=" << err.mps_at_max_abs << "\n";
    std::cout << "max_rel_at[idx=" << err.max_rel_idx << "] cpu=" << err.cpu_at_max_rel
              << ", mps=" << err.mps_at_max_rel << "\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "mps_vs_cpu_demo error: " << e.what() << "\n";
    std::cerr << "usage: mps_vs_cpu_demo [n inner_iters rounds] or [n m inner_iters rounds]\n";
    return 1;
  }
}
