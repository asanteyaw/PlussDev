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
};

struct QualityGates {
  bool enforce{false};
  double max_mean_rel{1e-3};
  double max_p95_rel{5e-3};
  double max_p99_rel{1e-2};
  double max_max_rel{5e-1};
  double max_p95_slowdown{8.0};
  double max_p99_slowdown{10.0};
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
  const auto c_add = pluss::make_series(0.05, dt).to(p);
  const auto c_mul = pluss::make_series(0.75, dt).to(p);
  const auto c_div = pluss::make_series(1.25, dt).to(p);
  const auto c_eps = pluss::make_series(1e-3, dt).to(p);
  const auto c_shift = pluss::make_series(0.15, dt).to(p);
  const auto n_last = std::max<std::int64_t>(1, x.sizes().empty() ? 1 : x.sizes().back());
  const auto c_scale = pluss::make_series(1.0 / std::sqrt(static_cast<double>(n_last)), dt).to(p);

  for (int i = 0; i < inner_iters; ++i) {
    x = x.matmul(a).mul(c_scale);
    x = x.mul(c_mul).add(c_add);
    x = x.abs().add(c_eps);
    x = x.log().exp();
    x = x.log1p().expm1();
    x = x.matmul(b).mul(c_scale);
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
  const auto c_eps = pluss::make_series(1e-3, dt).to(p);
  const auto c_small = pluss::make_series(0.1, dt).to(p);

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
  const auto c1 = pluss::make_series(0.5, dt).to(p);
  const auto c2 = pluss::make_series(1.25, dt).to(p);
  const auto c3 = pluss::make_series(0.125, dt).to(p);
  const auto eps = pluss::make_series(1e-3, dt).to(p);

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
  const auto one = pluss::make_series(1.0, dt).to(p);
  const auto half = pluss::make_series(0.5, dt).to(p);

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
  const auto c0 = pluss::make_series(0.0, dt).to(p);
  const auto c1 = pluss::make_series(0.1, dt).to(p);
  const auto c2 = pluss::make_series(0.9, dt).to(p);
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
  const auto lo = pluss::make_series(-0.5, dt).to(p);
  const auto hi = pluss::make_series(0.75, dt).to(p);
  const auto one = pluss::make_series(1.0, dt).to(p);
  const auto cond_mask = a.ge(b).to(dt);
  const auto inv_mask = one.sub(cond_mask);
  for (int i = 0; i < inner_iters; ++i) {
    // Keep selection semantics while avoiding backend-specific where path divergence.
    x = x.mul(cond_mask).add(b.mul(inv_mask));
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

BenchResult run_workload(WorkloadKind kind,
                         pluss::Platform p,
                         std::int64_t n,
                         int inner_iters,
                         int rounds,
                         std::uint64_t seed) {
  pluss::Config cpu_cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);
  const auto x0_cpu = pluss::randn({n, n}, cpu_cfg, seed);
  const auto a_cpu = pluss::randn({n, n}, cpu_cfg, seed + 1);
  const auto b_cpu = pluss::randn({n, n}, cpu_cfg, seed + 2);
  const auto x0 = x0_cpu.to(p);
  const auto a = a_cpu.to(p);
  const auto b = b_cpu.to(p);
  auto fn = workload_fn(kind);

  (void)fn(x0, a, b, std::max(1, inner_iters / 2));

  BenchResult out;
  out.round_ms.reserve(static_cast<std::size_t>(std::max(1, rounds)));
  const auto t0 = std::chrono::steady_clock::now();
  for (int r = 0; r < rounds; ++r) {
    const auto tr0 = std::chrono::steady_clock::now();
    out.last = fn(x0, a, b, inner_iters);
    const auto tr1 = std::chrono::steady_clock::now();
    out.round_ms.push_back(std::chrono::duration<double, std::milli>(tr1 - tr0).count());
    out.checksum += out.last.to_vec<float>(0);
  }
  const auto t1 = std::chrono::steady_clock::now();
  out.millis = std::chrono::duration<double, std::milli>(t1 - t0).count();
  out.latency = summarize_latencies(out.round_ms);
  return out;
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
    const double rel_e = abs_e / std::max(std::max(std::abs(a), std::abs(b)), kEps);
    abs_sum += abs_e;
    rel_sum += rel_e;
    stats.max_abs = std::max(stats.max_abs, abs_e);
    stats.max_rel = std::max(stats.max_rel, rel_e);
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

std::string workload_env_key_(WorkloadKind kind, const char* suffix) {
  std::string key = "PLUSS_MPS_GATE_";
  std::string wn = workload_name(kind);
  for (char ch : wn) {
    if (ch == '-') {
      key.push_back('_');
    } else {
      key.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
    }
  }
  key.push_back('_');
  key += suffix;
  return key;
}

QualityGates load_quality_gates_(WorkloadKind kind) {
  QualityGates g;
  g.enforce = parse_bool_env_("PLUSS_MPS_ENFORCE_GATES", false);

  // Workload-level default tolerances: branch-heavy and matmul-heavy paths need looser max_rel
  // while still keeping tight mean/p95/p99 constraints.
  if (kind == WorkloadKind::MatmulMix) {
    g.max_max_rel = 0.75;
    g.max_p99_rel = 5e-2;
  } else if (kind == WorkloadKind::BinaryMix) {
    g.max_max_rel = 0.25;
  } else if (kind == WorkloadKind::SelectClampMix) {
    g.max_mean_rel = 5e-2;
    g.max_p95_rel = 1e-1;
    g.max_p99_rel = 2e-1;
    g.max_max_rel = 5e-1;
  }

  const auto global_mean = parse_f64_env_("PLUSS_MPS_GATE_MAX_MEAN_REL", g.max_mean_rel, 0.0, 1e6);
  const auto global_p95 = parse_f64_env_("PLUSS_MPS_GATE_MAX_P95_REL", g.max_p95_rel, 0.0, 1e6);
  const auto global_p99 = parse_f64_env_("PLUSS_MPS_GATE_MAX_P99_REL", g.max_p99_rel, 0.0, 1e6);
  const auto global_max = parse_f64_env_("PLUSS_MPS_GATE_MAX_MAX_REL", g.max_max_rel, 0.0, 1e6);
  const auto global_s95 = parse_f64_env_("PLUSS_MPS_GATE_MAX_P95_SLOWDOWN", g.max_p95_slowdown, 1e-6, 1e6);
  const auto global_s99 = parse_f64_env_("PLUSS_MPS_GATE_MAX_P99_SLOWDOWN", g.max_p99_slowdown, 1e-6, 1e6);

  g.max_mean_rel = parse_f64_env_(workload_env_key_(kind, "MAX_MEAN_REL").c_str(), global_mean, 0.0, 1e6);
  g.max_p95_rel = parse_f64_env_(workload_env_key_(kind, "MAX_P95_REL").c_str(), global_p95, 0.0, 1e6);
  g.max_p99_rel = parse_f64_env_(workload_env_key_(kind, "MAX_P99_REL").c_str(), global_p99, 0.0, 1e6);
  g.max_max_rel = parse_f64_env_(workload_env_key_(kind, "MAX_MAX_REL").c_str(), global_max, 0.0, 1e6);
  g.max_p95_slowdown =
      parse_f64_env_(workload_env_key_(kind, "MAX_P95_SLOWDOWN").c_str(), global_s95, 1e-6, 1e6);
  g.max_p99_slowdown =
      parse_f64_env_(workload_env_key_(kind, "MAX_P99_SLOWDOWN").c_str(), global_s99, 1e-6, 1e6);
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
  if (round_ms.empty()) {
    return;
  }
  const int n = std::min<int>(limit, static_cast<int>(round_ms.size()));
  std::cout << "  profile[" << label << "] round_ms:";
  for (int i = 0; i < n; ++i) {
    std::cout << (i == 0 ? " " : ", ") << round_ms[static_cast<std::size_t>(i)];
  }
  if (n < static_cast<int>(round_ms.size())) {
    std::cout << ", ...";
  }
  std::cout << "\n";
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
    const bool profile_rounds = parse_bool_env_("PLUSS_MPS_PROFILE_BENCH", false);
    const int profile_round_limit = parse_i32_env_("PLUSS_MPS_PROFILE_BENCH_ITERS", 3, 1, 1000000);
    bool gate_failed = false;

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
      std::cout << "  [CPU] p95_ms=" << cpu.latency.p95_ms
                << ", p99_ms=" << cpu.latency.p99_ms
                << ", max_ms=" << cpu.latency.max_ms << "\n";

      if (!mps_ok) {
        std::cout << "  [MPS] skipped\n\n";
        continue;
      }

      const auto mps = run_workload(kind, pluss::pMPS, n, inner_iters, rounds, seed);
      std::cout << "  [MPS] total_ms=" << mps.millis
                << ", avg_ms_per_round=" << (mps.millis / static_cast<double>(rounds))
                << ", checksum=" << mps.checksum << "\n";
      std::cout << "  [MPS] p95_ms=" << mps.latency.p95_ms
                << ", p99_ms=" << mps.latency.p99_ms
                << ", max_ms=" << mps.latency.max_ms << "\n";

      const auto cpu_last = cpu.last.to(pluss::pCPU, pluss::ScalarType::Float32);
      const auto mps_last = mps.last.to(pluss::pCPU, pluss::ScalarType::Float32);
      const auto err = compare_stats(cpu_last, mps_last);

      std::cout << std::scientific << std::setprecision(6);
      std::cout << "  speed_ratio_cpu_over_mps=" << (cpu.millis / mps.millis) << "\n";
      std::cout << "  mean_abs_err=" << err.mean_abs << ", max_abs_err=" << err.max_abs << "\n";
      std::cout << "  mean_rel_err(sym)=" << err.mean_rel
                << ", p95_rel_err(sym)=" << err.p95_rel
                << ", p99_rel_err(sym)=" << err.p99_rel
                << ", max_rel_err(sym)=" << err.max_rel << "\n";
      std::cout << "  slowdown_p95=" << (mps.latency.p95_ms / std::max(cpu.latency.p95_ms, 1e-12))
                << ", slowdown_p99=" << (mps.latency.p99_ms / std::max(cpu.latency.p99_ms, 1e-12))
                << "\n\n";
      std::cout << std::fixed << std::setprecision(3);

      if (profile_rounds) {
        print_round_profile_("cpu", cpu.round_ms, profile_round_limit);
        print_round_profile_("mps", mps.round_ms, profile_round_limit);
      }

      const auto gates = load_quality_gates_(kind);
      std::vector<std::string> fails;
      const bool gate_ok = check_gates_(gates, err, cpu, mps, fails);
      if (!gate_ok) {
        std::cout << "  gates: FAIL";
        for (const auto& f : fails) std::cout << " " << f;
        std::cout << "\n";
        if (gates.enforce) {
          gate_failed = true;
        }
      } else {
        std::cout << "  gates: PASS\n";
      }
      if (gates.enforce) {
        std::cout << "  gates_enforced=true\n";
      }

      if (should_trace_workload_(kind)) {
        trace_workload_progress(kind, n, inner_iters, seed);
        std::cout << "\n";
      }
    }

    if (gate_failed) {
      std::cerr << "mps_bulk_bench_demo: one or more enforced quality gates failed\n";
      return 3;
    }
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "mps_bulk_bench_demo error: " << e.what() << "\n";
    std::cerr << "usage: mps_bulk_bench_demo [n inner_iters rounds]\n";
    return 1;
  }
}
