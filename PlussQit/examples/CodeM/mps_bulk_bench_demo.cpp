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

struct BenchResult {
  double millis{0.0};
  float checksum{0.0f};
  pluss::Series last{pluss::Series::undefined()};
};

struct ErrorStats {
  double mean_abs{0.0};
  double max_abs{0.0};
  double mean_rel{0.0};
  double p95_rel{0.0};
  double p99_rel{0.0};
};

enum class WorkloadKind : std::uint8_t {
  MatmulMix,
  UnaryMix,
  BinaryMix,
  PredicateMix,
  ActivationMix,
  SelectClampMix,
};

const char* workload_name(WorkloadKind k) {
  switch (k) {
    case WorkloadKind::MatmulMix: return "matmul_mix";
    case WorkloadKind::UnaryMix: return "unary_mix";
    case WorkloadKind::BinaryMix: return "binary_mix";
    case WorkloadKind::PredicateMix: return "predicate_mix";
    case WorkloadKind::ActivationMix: return "activation_mix";
    case WorkloadKind::SelectClampMix: return "select_clamp_mix";
    default: return "unknown";
  }
}

pluss::Series workload_matmul_mix(pluss::Series x,
                                  const pluss::Series& a,
                                  const pluss::Series& b,
                                  int inner_iters) {
  const auto p = x.platform();
  const auto dt = pluss::ScalarType::Float32;
  const auto c_add = pluss::value(0.05, dt).to(p);
  const auto c_mul = pluss::value(0.75, dt).to(p);
  const auto c_div = pluss::value(1.25, dt).to(p);
  const auto c_eps = pluss::value(1e-3, dt).to(p);
  const auto c_shift = pluss::value(0.15, dt).to(p);

  for (int i = 0; i < inner_iters; ++i) {
    x = x.matmul(a);
    x = x.mul(c_mul).add(c_add);
    x = x.abs().add(c_eps);
    x = x.log().exp();
    x = x.log1p().expm1();
    x = x.matmul(b);
    x = x.abs().add(c_shift).sqrt();
    x = x.mul(c_div).div(c_mul).add(c_add);
  }
  return x;
}

pluss::Series workload_unary_mix(pluss::Series x,
                                 const pluss::Series&,
                                 const pluss::Series&,
                                 int inner_iters) {
  const auto p = x.platform();
  const auto dt = pluss::ScalarType::Float32;
  const auto c_eps = pluss::value(1e-3, dt).to(p);
  const auto c_small = pluss::value(0.1, dt).to(p);

  for (int i = 0; i < inner_iters; ++i) {
    x = x.mul(c_small);
    x = x.sin().cos().tanh();
    const auto u = x.tanh();
    x = u.asin().add(u.acos()).add(u.atan());
    x = x.sinh().cosh();
    x = x.abs().add(c_eps).reciprocal();
    x = x.abs().add(c_eps).sqrt();
    x = x.floor().ceil().round().trunc();
    x = x.log1p().expm1();
  }
  return x;
}

pluss::Series workload_binary_mix(pluss::Series x,
                                  const pluss::Series& a,
                                  const pluss::Series& b,
                                  int inner_iters) {
  const auto p = x.platform();
  const auto dt = pluss::ScalarType::Float32;
  const auto c1 = pluss::value(0.5, dt).to(p);
  const auto c2 = pluss::value(1.25, dt).to(p);
  const auto c3 = pluss::value(0.125, dt).to(p);
  const auto eps = pluss::value(1e-3, dt).to(p);

  for (int i = 0; i < inner_iters; ++i) {
    const auto y = a.mul(c1).add(b.mul(c2));
    const auto base = x.abs().add(eps);
    const auto expo = y.abs().mul(c3).add(c1);
    x = base.pow(expo);
    x = x.add(y).sub(base).mul(c1).div(expo.add(c2));
  }
  return x;
}

pluss::Series workload_predicate_mix(pluss::Series x,
                                     const pluss::Series&,
                                     const pluss::Series&,
                                     int inner_iters) {
  const auto p = x.platform();
  const auto dt = pluss::ScalarType::Float32;
  const auto one = pluss::value(1.0, dt).to(p);
  const auto half = pluss::value(0.5, dt).to(p);

  for (int i = 0; i < inner_iters; ++i) {
    const auto z = x.div(x.sub(x));
    const auto nan_m = z.isnan().to(dt);
    const auto inf_m = z.isinf().to(dt);
    const auto fin_m = z.isfinite().to(dt);
    x = nan_m.add(inf_m).add(fin_m).add(one);
    x = x.mul(half);
  }
  return x;
}

pluss::Series workload_activation_mix(pluss::Series x,
                                      const pluss::Series& a,
                                      const pluss::Series& b,
                                      int inner_iters) {
  const auto p = x.platform();
  const auto dt = pluss::ScalarType::Float32;
  const auto c0 = pluss::value(0.0, dt).to(p);
  const auto c1 = pluss::value(0.1, dt).to(p);
  const auto c2 = pluss::value(0.9, dt).to(p);
  for (int i = 0; i < inner_iters; ++i) {
    x = x.add(a.mul(c1)).sub(b.mul(c1));
    x = x.relu();
    x = x.sigmoid();
    x = x.maximum(c0).minimum(c2);
  }
  return x;
}

pluss::Series workload_select_clamp_mix(pluss::Series x,
                                        const pluss::Series& a,
                                        const pluss::Series& b,
                                        int inner_iters) {
  const auto p = x.platform();
  const auto dt = pluss::ScalarType::Float32;
  const auto lo = pluss::value(-0.5, dt).to(p);
  const auto hi = pluss::value(0.75, dt).to(p);
  for (int i = 0; i < inner_iters; ++i) {
    const auto cond = x.ge(a);
    x = x.where(cond, b);
    x = x.maximum(lo).minimum(hi);
    x = x.clamp_min(-0.25).clamp_max(0.5);
  }
  return x;
}

using WorkloadFn = pluss::Series(*)(pluss::Series, const pluss::Series&, const pluss::Series&, int);

WorkloadFn workload_fn(WorkloadKind k) {
  switch (k) {
    case WorkloadKind::MatmulMix: return &workload_matmul_mix;
    case WorkloadKind::UnaryMix: return &workload_unary_mix;
    case WorkloadKind::BinaryMix: return &workload_binary_mix;
    case WorkloadKind::PredicateMix: return &workload_predicate_mix;
    case WorkloadKind::ActivationMix: return &workload_activation_mix;
    case WorkloadKind::SelectClampMix: return &workload_select_clamp_mix;
    default: return &workload_matmul_mix;
  }
}

BenchResult run_workload(WorkloadKind kind,
                         pluss::Platform p,
                         std::int64_t n,
                         int inner_iters,
                         int rounds,
                         std::uint64_t seed) {
  pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(p);
  const auto x0 = pluss::randn({n, n}, cfg, seed);
  const auto a = pluss::randn({n, n}, cfg, seed + 1);
  const auto b = pluss::randn({n, n}, cfg, seed + 2);
  auto fn = workload_fn(kind);

  (void)fn(x0, a, b, std::max(1, inner_iters / 2));

  BenchResult out;
  const auto t0 = std::chrono::steady_clock::now();
  for (int r = 0; r < rounds; ++r) {
    out.last = fn(x0, a, b, inner_iters);
    out.checksum += out.last.to_vec<float>(0);
  }
  const auto t1 = std::chrono::steady_clock::now();
  out.millis = std::chrono::duration<double, std::milli>(t1 - t0).count();
  return out;
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
    const double rel_e = abs_e / std::max(std::max(std::abs(a), std::abs(b)), kEps);
    abs_sum += abs_e;
    rel_sum += rel_e;
    stats.max_abs = std::max(stats.max_abs, abs_e);
    rel_vals.push_back(rel_e);
  }

  stats.mean_abs = abs_sum / static_cast<double>(c.data.size());
  stats.mean_rel = rel_sum / static_cast<double>(c.data.size());

  auto rel95 = rel_vals;
  auto rel99 = rel_vals;
  const std::size_t i95 = (rel95.size() * 95) / 100;
  const std::size_t i99 = (rel99.size() * 99) / 100;
  std::nth_element(rel95.begin(), rel95.begin() + static_cast<std::ptrdiff_t>(i95), rel95.end());
  std::nth_element(rel99.begin(), rel99.begin() + static_cast<std::ptrdiff_t>(i99), rel99.end());
  stats.p95_rel = rel95[i95];
  stats.p99_rel = rel99[i99];
  return stats;
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

int parse_i32_env_(const char* key, int def, int lo, int hi) noexcept {
  const char* raw = std::getenv(key);
  if (raw == nullptr || *raw == '\0') return def;
  char* end = nullptr;
  const long long v = std::strtoll(raw, &end, 10);
  if (end == raw || (end != nullptr && *end != '\0')) return def;
  if (v < lo || v > hi) return def;
  return static_cast<int>(v);
}

bool should_trace_workload_(WorkloadKind k) noexcept {
  const bool trace_enabled = parse_bool_env_("PLUSS_MPS_TRACE_PARITY", false);
  if (!trace_enabled) {
    return false;
  }
  const char* raw = std::getenv("PLUSS_MPS_TRACE_WORKLOAD");
  if (raw == nullptr || *raw == '\0') {
    return k == WorkloadKind::MatmulMix;
  }
  if (ieq_(raw, "all")) {
    return true;
  }
  return ieq_(raw, workload_name(k));
}

void trace_workload_progress(WorkloadKind kind, std::int64_t n, int inner_iters, std::uint64_t seed) {
  const int trace_iters = parse_i32_env_("PLUSS_MPS_TRACE_PARITY_ITERS", inner_iters, 1, std::max(1, inner_iters));
  std::cout << "  parity_progress[" << workload_name(kind) << "]\n";
  std::cout << std::scientific << std::setprecision(6);
  for (int it = 1; it <= trace_iters; ++it) {
    const auto cpu = run_workload(kind, pluss::pCPU, n, it, 1, seed);
    const auto mps = run_workload(kind, pluss::pMPS, n, it, 1, seed);
    const auto cpu_last = cpu.last.to(pluss::pCPU, pluss::ScalarType::Float32);
    const auto mps_last = mps.last.to(pluss::pCPU, pluss::ScalarType::Float32);
    const auto err = compare_stats(cpu_last, mps_last);
    std::cout << "    iter=" << it
              << " mean_abs=" << err.mean_abs
              << " max_abs=" << err.max_abs
              << " mean_rel(sym)=" << err.mean_rel
              << " p99_rel(sym)=" << err.p99_rel << "\n";
  }
  std::cout << std::fixed << std::setprecision(3);
}

} // namespace

int main(int argc, char** argv) {
  try {
    std::int64_t n = 1024;
    int inner_iters = 6;
    int rounds = 3;

    if (argc > 1) n = parse_i64(argv[1], "n");
    if (argc > 2) inner_iters = parse_i32(argv[2], "inner_iters");
    if (argc > 3) rounds = parse_i32(argv[3], "rounds");

    if (n <= 0) {
      throw std::runtime_error("n must be positive");
    }

    std::cout << "Bulk benchmark settings\n";
    std::cout << "shape=(" << n << ", " << n << "), inner_iters=" << inner_iters
              << ", rounds=" << rounds << "\n";
    const bool mps_ok = pluss::mps::is_ready();
    std::cout << "MPS ready: " << (mps_ok ? "true" : "false") << "\n\n";

    const std::vector<WorkloadKind> workloads{
      WorkloadKind::MatmulMix,
      WorkloadKind::UnaryMix,
      WorkloadKind::BinaryMix,
      WorkloadKind::PredicateMix,
      WorkloadKind::ActivationMix,
      WorkloadKind::SelectClampMix,
    };

    std::cout << std::fixed << std::setprecision(3);
    for (std::size_t wi = 0; wi < workloads.size(); ++wi) {
      const auto kind = workloads[wi];
      const auto seed = static_cast<std::uint64_t>(42 + wi * 100);

      const auto cpu = run_workload(kind, pluss::pCPU, n, inner_iters, rounds, seed);
      std::cout << "[" << workload_name(kind) << "]\n";
      std::cout << "  [CPU] total_ms=" << cpu.millis
                << ", avg_ms_per_round=" << (cpu.millis / static_cast<double>(rounds))
                << ", checksum=" << cpu.checksum << "\n";

      if (!mps_ok) {
        std::cout << "  [MPS] skipped\n\n";
        continue;
      }

      const auto mps = run_workload(kind, pluss::pMPS, n, inner_iters, rounds, seed);
      std::cout << "  [MPS] total_ms=" << mps.millis
                << ", avg_ms_per_round=" << (mps.millis / static_cast<double>(rounds))
                << ", checksum=" << mps.checksum << "\n";

      const auto cpu_last = cpu.last.to(pluss::pCPU, pluss::ScalarType::Float32);
      const auto mps_last = mps.last.to(pluss::pCPU, pluss::ScalarType::Float32);
      const auto err = compare_stats(cpu_last, mps_last);

      std::cout << std::scientific << std::setprecision(6);
      std::cout << "  speed_ratio_cpu_over_mps=" << (cpu.millis / mps.millis) << "\n";
      std::cout << "  mean_abs_err=" << err.mean_abs << ", max_abs_err=" << err.max_abs << "\n";
      std::cout << "  mean_rel_err(sym)=" << err.mean_rel
                << ", p95_rel_err(sym)=" << err.p95_rel
                << ", p99_rel_err(sym)=" << err.p99_rel << "\n\n";
      std::cout << std::fixed << std::setprecision(3);

      if (should_trace_workload_(kind)) {
        trace_workload_progress(kind, n, inner_iters, seed);
        std::cout << "\n";
      }
    }

    return 0;
  } catch (const std::exception& e) {
    std::cerr << "mps_bulk_bench_demo error: " << e.what() << "\n";
    std::cerr << "usage: mps_bulk_bench_demo [n inner_iters rounds]\n";
    return 1;
  }
}
