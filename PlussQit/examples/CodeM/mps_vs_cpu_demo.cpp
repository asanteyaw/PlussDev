#include "pluss/codem/codem.h"
#include "pluss/codem/series_factory.h"
#include <pluss/pluss.h>

#include <algorithm>
#include <cctype>
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

struct LatencyStats {
  double mean_ms{0.0};
  double p95_ms{0.0};
  double p99_ms{0.0};
  double max_ms{0.0};
};

struct BenchResult {
  double millis{0.0};
  float checksum{0.0f};
  pluss::Series last{pluss::Series::undefined()};
  std::vector<double> round_ms{};
  LatencyStats latency{};
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

struct QualityGates {
  bool enforce{false};
  double max_mean_rel{1e-3};
  double max_p95_rel{5e-3};
  double max_p99_rel{1e-2};
  double max_max_rel{1e-1};
  double max_p95_slowdown{8.0};
  double max_p99_slowdown{10.0};
};

pluss::Series complex_chain(pluss::Series x,
                            const pluss::Series& w1,
                            const pluss::Series& w2,
                            int inner_iters) {
  const auto p = x.platform();
  const auto dt = pluss::ScalarType::Float32;

  const auto c_add = pluss::make_series(0.05, dt).to(p);
  const auto c_mul = pluss::make_series(0.75, dt).to(p);
  const auto c_div = pluss::make_series(1.25, dt).to(p);
  const auto c_eps = pluss::make_series(1e-3, dt).to(p);
  const auto c_shift = pluss::make_series(0.15, dt).to(p);
  const auto n_last = std::max<std::int64_t>(1, x.sizes().empty() ? 1 : x.sizes().back());
  const auto c_scale = pluss::make_series(1.0 / std::sqrt(static_cast<double>(n_last)), dt).to(p);

  for (int i = 0; i < inner_iters; ++i) {
    x = x.matmul(w1).mul(c_scale);
    x = x.mul(c_mul).add(c_add);
    x = x.abs().add(c_eps);
    x = x.log().exp();
    x = x.log1p().expm1();
    x = x.matmul(w2).mul(c_scale);
    x = x.abs().add(c_shift).sqrt();
    x = x.mul(c_div).div(c_mul).add(c_add);
  }
  return x;
}

LatencyStats summarize_latencies(const std::vector<double>& values_ms) {
  LatencyStats out;
  if (values_ms.empty()) return out;
  double sum = 0.0;
  for (double v : values_ms) {
    sum += v;
    out.max_ms = std::max(out.max_ms, v);
  }
  out.mean_ms = sum / static_cast<double>(values_ms.size());
  auto p95 = values_ms;
  auto p99 = values_ms;
  const std::size_t i95 = (p95.size() * 95) / 100;
  const std::size_t i99 = (p99.size() * 99) / 100;
  std::nth_element(p95.begin(), p95.begin() + static_cast<std::ptrdiff_t>(i95), p95.end());
  std::nth_element(p99.begin(), p99.begin() + static_cast<std::ptrdiff_t>(i99), p99.end());
  out.p95_ms = p95[i95];
  out.p99_ms = p99[i99];
  return out;
}

BenchResult run_bench(pluss::Platform p, std::int64_t n, int inner_iters, int rounds, std::uint64_t seed) {
  pluss::Config cpu_cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);
  const auto x0_cpu = pluss::randn({n, n}, cpu_cfg, seed);
  const auto w1_cpu = pluss::randn({n, n}, cpu_cfg, seed + 1);
  const auto w2_cpu = pluss::randn({n, n}, cpu_cfg, seed + 2);
  const auto x0 = x0_cpu.to(p);
  const auto w1 = w1_cpu.to(p);
  const auto w2 = w2_cpu.to(p);

  // Warm-up to reduce one-time setup noise.
  (void)complex_chain(x0, w1, w2, std::max(1, inner_iters / 2));

  BenchResult out;
  out.round_ms.reserve(static_cast<std::size_t>(std::max(1, rounds)));
  const auto t0 = std::chrono::steady_clock::now();
  for (int r = 0; r < rounds; ++r) {
    const auto tr0 = std::chrono::steady_clock::now();
    out.last = complex_chain(x0, w1, w2, inner_iters);
    const auto tr1 = std::chrono::steady_clock::now();
    out.round_ms.push_back(std::chrono::duration<double, std::milli>(tr1 - tr0).count());
    out.checksum += out.last.to_vec<float>(0);
  }
  const auto t1 = std::chrono::steady_clock::now();
  out.millis = std::chrono::duration<double, std::milli>(t1 - t0).count();
  out.latency = summarize_latencies(out.round_ms);
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

bool ieq_(const char* a, const char* b) noexcept {
  if (a == nullptr || b == nullptr) return false;
  while (*a != '\0' && *b != '\0') {
    if (std::tolower(static_cast<unsigned char>(*a)) != std::tolower(static_cast<unsigned char>(*b))) {
      return false;
    }
    ++a;
    ++b;
  }
  return *a == '\0' && *b == '\0';
}

bool parse_bool_env_(const char* key, bool def) noexcept {
  const char* raw = std::getenv(key);
  if (raw == nullptr || *raw == '\0') return def;
  if (ieq_(raw, "1") || ieq_(raw, "true") || ieq_(raw, "yes") || ieq_(raw, "on")) return true;
  if (ieq_(raw, "0") || ieq_(raw, "false") || ieq_(raw, "no") || ieq_(raw, "off")) return false;
  return def;
}

double parse_f64_env_(const char* key, double def, double lo, double hi) noexcept {
  const char* raw = std::getenv(key);
  if (raw == nullptr || *raw == '\0') return def;
  char* end = nullptr;
  const double v = std::strtod(raw, &end);
  if (end == raw || (end != nullptr && *end != '\0')) return def;
  if (!(v >= lo && v <= hi)) return def;
  return v;
}

int parse_i32_env_(const char* key, int def, int lo, int hi) noexcept {
  const char* raw = std::getenv(key);
  if (raw == nullptr || *raw == '\0') return def;
  char* end = nullptr;
  const long long v = std::strtoll(raw, &end, 10);
  if (end == raw || (end != nullptr && *end != '\0')) return def;
  if (v < lo || v > hi) return def;
  return static_cast<int>(v);
}

QualityGates load_quality_gates_() {
  QualityGates g;
  g.enforce = parse_bool_env_("PLUSS_MPS_ENFORCE_GATES", false);
  g.max_mean_rel = parse_f64_env_("PLUSS_MPS_GATE_MAX_MEAN_REL", g.max_mean_rel, 0.0, 1e6);
  g.max_p95_rel = parse_f64_env_("PLUSS_MPS_GATE_MAX_P95_REL", g.max_p95_rel, 0.0, 1e6);
  g.max_p99_rel = parse_f64_env_("PLUSS_MPS_GATE_MAX_P99_REL", g.max_p99_rel, 0.0, 1e6);
  g.max_max_rel = parse_f64_env_("PLUSS_MPS_GATE_MAX_MAX_REL", g.max_max_rel, 0.0, 1e6);
  g.max_p95_slowdown = parse_f64_env_("PLUSS_MPS_GATE_MAX_P95_SLOWDOWN", g.max_p95_slowdown, 1e-6, 1e6);
  g.max_p99_slowdown = parse_f64_env_("PLUSS_MPS_GATE_MAX_P99_SLOWDOWN", g.max_p99_slowdown, 1e-6, 1e6);
  return g;
}

bool check_gates_(const QualityGates& g,
                  const ErrorStats& err,
                  const BenchResult& cpu,
                  const BenchResult& mps,
                  std::vector<std::string>& fails) {
  const double kEps = 1e-12;
  const double p95_slow = mps.latency.p95_ms / std::max(cpu.latency.p95_ms, kEps);
  const double p99_slow = mps.latency.p99_ms / std::max(cpu.latency.p99_ms, kEps);
  if (err.mean_rel > g.max_mean_rel) fails.push_back("mean_rel");
  if (err.p95_rel > g.max_p95_rel) fails.push_back("p95_rel");
  if (err.p99_rel > g.max_p99_rel) fails.push_back("p99_rel");
  if (err.max_rel > g.max_max_rel) fails.push_back("max_rel");
  if (p95_slow > g.max_p95_slowdown) fails.push_back("p95_slowdown");
  if (p99_slow > g.max_p99_slowdown) fails.push_back("p99_slowdown");
  return fails.empty();
}

void print_round_profile_(const char* label, const std::vector<double>& round_ms, int limit) {
  if (round_ms.empty()) return;
  const int n = std::min<int>(limit, static_cast<int>(round_ms.size()));
  std::cout << "profile[" << label << "] round_ms:";
  for (int i = 0; i < n; ++i) {
    std::cout << (i == 0 ? " " : ", ") << round_ms[static_cast<std::size_t>(i)];
  }
  if (n < static_cast<int>(round_ms.size())) std::cout << ", ...";
  std::cout << "\n";
}

ErrorStats compare_stats(const pluss::Series& cpu_last, const pluss::Series& mps_last) {
  const auto c = cpu_last.host<float>().array();
  const auto g = mps_last.host<float>().array();
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

void trace_parity_chain(std::int64_t n, int trace_iters, std::uint64_t seed) {
  if (!pluss::mps::is_ready()) return;

  pluss::Config cpu_cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);

  auto x_cpu = pluss::randn({n, n}, cpu_cfg, seed);
  auto w1_cpu = pluss::randn({n, n}, cpu_cfg, seed + 1);
  auto w2_cpu = pluss::randn({n, n}, cpu_cfg, seed + 2);
  auto x_mps = x_cpu.to(pluss::pMPS);
  auto w1_mps = w1_cpu.to(pluss::pMPS);
  auto w2_mps = w2_cpu.to(pluss::pMPS);

  const auto dt = pluss::ScalarType::Float32;
  const auto c_add_cpu = pluss::make_series(0.05, dt).to(pluss::pCPU);
  const auto c_mul_cpu = pluss::make_series(0.75, dt).to(pluss::pCPU);
  const auto c_div_cpu = pluss::make_series(1.25, dt).to(pluss::pCPU);
  const auto c_eps_cpu = pluss::make_series(1e-3, dt).to(pluss::pCPU);
  const auto c_shift_cpu = pluss::make_series(0.15, dt).to(pluss::pCPU);
  const auto c_add_mps = c_add_cpu.to(pluss::pMPS);
  const auto c_mul_mps = c_mul_cpu.to(pluss::pMPS);
  const auto c_div_mps = c_div_cpu.to(pluss::pMPS);
  const auto c_eps_mps = c_eps_cpu.to(pluss::pMPS);
  const auto c_shift_mps = c_shift_cpu.to(pluss::pMPS);

  auto report = [&](const char* stage, int iter) {
    const auto cpu_last = x_cpu.to(pluss::pCPU, pluss::ScalarType::Float32);
    const auto mps_last = x_mps.to(pluss::pCPU, pluss::ScalarType::Float32);
    const auto err = compare_stats(cpu_last, mps_last);
    std::cout << "  iter=" << iter << " stage=" << stage
              << " mean_abs=" << err.mean_abs
              << " max_abs=" << err.max_abs
              << " mean_rel(sym)=" << err.mean_rel
              << " p99_rel(sym)=" << err.p99_rel << "\n";
  };

  std::cout << "\nParity trace (complex_chain)\n";
  std::cout << std::scientific << std::setprecision(6);
  for (int i = 1; i <= trace_iters; ++i) {
    x_cpu = x_cpu.matmul(w1_cpu);
    x_mps = x_mps.matmul(w1_mps);
    report("matmul_w1", i);

    x_cpu = x_cpu.mul(c_mul_cpu).add(c_add_cpu);
    x_mps = x_mps.mul(c_mul_mps).add(c_add_mps);
    report("affine_1", i);

    x_cpu = x_cpu.abs().add(c_eps_cpu);
    x_mps = x_mps.abs().add(c_eps_mps);
    report("abs_add_eps", i);

    x_cpu = x_cpu.log().exp();
    x_mps = x_mps.log().exp();
    report("log_exp", i);

    x_cpu = x_cpu.log1p().expm1();
    x_mps = x_mps.log1p().expm1();
    report("log1p_expm1", i);

    x_cpu = x_cpu.matmul(w2_cpu);
    x_mps = x_mps.matmul(w2_mps);
    report("matmul_w2", i);

    x_cpu = x_cpu.abs().add(c_shift_cpu).sqrt();
    x_mps = x_mps.abs().add(c_shift_mps).sqrt();
    report("abs_shift_sqrt", i);

    x_cpu = x_cpu.mul(c_div_cpu).div(c_mul_cpu).add(c_add_cpu);
    x_mps = x_mps.mul(c_div_mps).div(c_mul_mps).add(c_add_mps);
    report("affine_2", i);
  }
  std::cout << std::fixed << std::setprecision(3);
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
    std::cout << "MPS ready: " << (pluss::mps::is_ready() ? "true" : "false") << "\n\n";

    const std::uint64_t seed = 42;
    const auto cpu = run_bench(pluss::pCPU, n, inner_iters, rounds, seed);
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "[CPU] total_ms=" << cpu.millis
              << ", avg_ms_per_round=" << (cpu.millis / static_cast<double>(rounds))
              << ", checksum=" << cpu.checksum << "\n";
    std::cout << "[CPU] p95_ms=" << cpu.latency.p95_ms
              << ", p99_ms=" << cpu.latency.p99_ms
              << ", max_ms=" << cpu.latency.max_ms << "\n";

    if (!pluss::mps::is_ready()) {
      std::cout << "\nMPS benchmark skipped because backend is not ready in this runtime/build.\n";
      return 0;
    }

    const auto mps = run_bench(pluss::pMPS, n, inner_iters, rounds, seed);
    std::cout << "[MPS] total_ms=" << mps.millis
              << ", avg_ms_per_round=" << (mps.millis / static_cast<double>(rounds))
              << ", checksum=" << mps.checksum << "\n";
    std::cout << "[MPS] p95_ms=" << mps.latency.p95_ms
              << ", p99_ms=" << mps.latency.p99_ms
              << ", max_ms=" << mps.latency.max_ms << "\n";

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
    std::cout << "slowdown_p95=" << (mps.latency.p95_ms / std::max(cpu.latency.p95_ms, 1e-12))
              << ", slowdown_p99=" << (mps.latency.p99_ms / std::max(cpu.latency.p99_ms, 1e-12))
              << "\n";

    if (parse_bool_env_("PLUSS_MPS_PROFILE_BENCH", false)) {
      const int limit = parse_i32_env_("PLUSS_MPS_PROFILE_BENCH_ITERS", 3, 1, std::max(1, rounds));
      print_round_profile_("cpu", cpu.round_ms, limit);
      print_round_profile_("mps", mps.round_ms, limit);
    }

    const auto gates = load_quality_gates_();
    std::vector<std::string> fails;
    if (!check_gates_(gates, err, cpu, mps, fails)) {
      std::cout << "gates: FAIL";
      for (const auto& f : fails) std::cout << " " << f;
      std::cout << "\n";
      if (gates.enforce) {
        std::cerr << "mps_vs_cpu_demo: enforced quality gates failed\n";
        return 3;
      }
    } else {
      std::cout << "gates: PASS\n";
    }

    if (parse_bool_env_("PLUSS_MPS_TRACE_PARITY", false)) {
      const int trace_iters = parse_i32_env_("PLUSS_MPS_TRACE_PARITY_ITERS", 1, 1, std::max(1, inner_iters));
      trace_parity_chain(n, trace_iters, seed);
    }
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "mps_vs_cpu_demo error: " << e.what() << "\n";
    std::cerr << "usage: mps_vs_cpu_demo [n inner_iters rounds] or [n m inner_iters rounds]\n";
    return 1;
  }
}
