#include <pluss/pluss.h>
#include <pluss/codem/torch_interop.h>

#include <torch/torch.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

using clock_t = std::chrono::steady_clock;

struct Row {
  std::string op;
  double series_ms;
  double torch_ms;
  int iters;
};

struct CliOptions {
  double target_score = 90.0;
  bool enforce_target = false;
  bool emit_csv = false;
};

template <class F>
double time_ms(int iters, F&& fn) {
  const auto t0 = clock_t::now();
  for (int i = 0; i < iters; ++i) {
    fn();
  }
  const auto dt = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(clock_t::now() - t0);
  return dt.count();
}

float first_f32(const pluss::Series& s) {
  const auto* p = reinterpret_cast<const float*>(s.raw_data());
  return p ? p[0] : 0.0f;
}

bool parse_double_opt(const std::string& value, double* out) {
  if (!out) return false;
  char* end = nullptr;
  const double v = std::strtod(value.c_str(), &end);
  if (end == value.c_str() || *end != '\0' || !std::isfinite(v)) return false;
  *out = v;
  return true;
}

CliOptions parse_args(int argc, char** argv) {
  CliOptions opts;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--enforce-target") {
      opts.enforce_target = true;
      continue;
    }
    if (arg == "--emit-csv") {
      opts.emit_csv = true;
      continue;
    }
    if (arg == "--help") {
      std::cout << "Usage: bench_torch_parity [--target-score=<pct>] [--enforce-target] [--emit-csv]\n";
      std::exit(0);
    }
    const std::string prefix = "--target-score=";
    if (arg.rfind(prefix, 0) == 0) {
      const std::string v = arg.substr(prefix.size());
      double parsed = opts.target_score;
      if (!parse_double_opt(v, &parsed)) {
        std::cerr << "Invalid --target-score value: " << v << "\n";
        std::exit(2);
      }
      opts.target_score = parsed;
      opts.enforce_target = true;
      continue;
    }
    std::cerr << "Unknown argument: " << arg << "\n";
    std::exit(2);
  }
  return opts;
}

} // namespace

int main(int argc, char** argv) {
  const CliOptions opts = parse_args(argc, argv);
  torch::manual_seed(42);

  constexpr std::int64_t N = 10'000'000;
  constexpr int iters_bin = 30;
  constexpr int iters_unary = 20;
  constexpr int iters_reduce = 8;
  constexpr int iters_index = 10;
  constexpr int iters_mm = 8;

  auto a_t = torch::full({N}, 1.25f, torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU));
  auto b_t = torch::full({N}, 2.00f, torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU));
  auto m_t = torch::randn({N}, torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU));

  auto a = pluss::codem::from_torch(a_t);
  auto b = pluss::codem::from_torch(b_t);
  auto m = pluss::codem::from_torch(m_t);

  volatile float sink = 0.0f;

  // Warmup
  auto c = a.add(b);
  c = c.mul(a);
  c = c.exp();
  c = m.relu();
  c = m.sum();
  auto c_t = a_t + b_t;
  c_t = c_t * a_t;
  c_t = torch::exp(c_t);
  c_t = torch::relu(m_t);
  c_t = torch::sum(m_t);

  std::vector<Row> rows;

  rows.push_back(Row{
      "add",
      time_ms(iters_bin, [&] {
        c = a.add(b);
        sink += first_f32(c);
      }),
      time_ms(iters_bin, [&] {
        c_t = a_t + b_t;
        sink += c_t[0].item<float>();
      }),
      iters_bin});

  rows.push_back(Row{
      "mul",
      time_ms(iters_bin, [&] {
        c = a.mul(b);
        sink += first_f32(c);
      }),
      time_ms(iters_bin, [&] {
        c_t = a_t * b_t;
        sink += c_t[0].item<float>();
      }),
      iters_bin});

  rows.push_back(Row{
      "exp",
      time_ms(iters_unary, [&] {
        c = a.exp();
        sink += first_f32(c);
      }),
      time_ms(iters_unary, [&] {
        c_t = torch::exp(a_t);
        sink += c_t[0].item<float>();
      }),
      iters_unary});

  rows.push_back(Row{
      "relu",
      time_ms(iters_unary, [&] {
        c = m.relu();
        sink += first_f32(c);
      }),
      time_ms(iters_unary, [&] {
        c_t = torch::relu(m_t);
        sink += c_t[0].item<float>();
      }),
      iters_unary});

  rows.push_back(Row{
      "sum",
      time_ms(iters_reduce, [&] {
        c = m.sum();
        sink += first_f32(c);
      }),
      time_ms(iters_reduce, [&] {
        c_t = torch::sum(m_t);
        sink += c_t.item<float>();
      }),
      iters_reduce});

  constexpr std::int64_t R = 1024;
  constexpr std::int64_t C = 1024;
  auto x_t = torch::rand({R, C}, torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU));
  auto idx_t = torch::randint(0, C, {R, C}, torch::TensorOptions().dtype(torch::kInt64).device(torch::kCPU));
  auto src_t = torch::rand({R, C}, torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU));

  auto x = pluss::codem::from_torch(x_t);
  auto idx = pluss::codem::from_torch(idx_t);
  auto src = pluss::codem::from_torch(src_t);

  rows.push_back(Row{
      "gather",
      time_ms(iters_index, [&] {
        c = x.gather(1, idx);
        sink += first_f32(c);
      }),
      time_ms(iters_index, [&] {
        c_t = torch::gather(x_t, 1, idx_t);
        sink += c_t[0][0].item<float>();
      }),
      iters_index});

  rows.push_back(Row{
      "scatter",
      time_ms(iters_index, [&] {
        c = x.scatter(1, idx, src);
        sink += first_f32(c);
      }),
      time_ms(iters_index, [&] {
        c_t = x_t.scatter(1, idx_t, src_t);
        sink += c_t[0][0].item<float>();
      }),
      iters_index});

  constexpr std::int64_t M = 512;
  constexpr std::int64_t K = 512;
  constexpr std::int64_t P = 512;
  auto lhs_t = torch::rand({M, K}, torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU));
  auto rhs_t = torch::rand({K, P}, torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU));
  auto lhs = pluss::codem::from_torch(lhs_t);
  auto rhs = pluss::codem::from_torch(rhs_t);

  rows.push_back(Row{
      "matmul",
      time_ms(iters_mm, [&] {
        c = lhs.matmul(rhs);
        sink += first_f32(c);
      }),
      time_ms(iters_mm, [&] {
        c_t = torch::matmul(lhs_t, rhs_t);
        sink += c_t[0][0].item<float>();
      }),
      iters_mm});

  std::cout << std::left << std::setw(10) << "op"
            << std::right << std::setw(14) << "Series ms"
            << std::setw(14) << "Torch ms"
            << std::setw(14) << "delta %"
            << std::setw(14) << "S/T"
            << std::setw(14) << "S iter ms"
            << std::setw(14) << "T iter ms"
            << "\n";

  int within_125 = 0;
  int within_150 = 0;
  int within_200 = 0;
  double score_sum = 0.0;

  for (const auto& r : rows) {
    const double delta_pct = (r.series_ms / r.torch_ms - 1.0) * 100.0;
    const double ratio = r.series_ms / r.torch_ms;
    if (ratio <= 1.25) ++within_125;
    if (ratio <= 1.50) ++within_150;
    if (ratio <= 2.00) ++within_200;
    const double op_score = std::clamp(r.torch_ms / std::max(r.series_ms, std::numeric_limits<double>::min()), 0.0, 1.0);
    score_sum += op_score;
    std::cout << std::left << std::setw(10) << r.op
              << std::right << std::setw(14) << std::fixed << std::setprecision(3) << r.series_ms
              << std::setw(14) << r.torch_ms
              << std::setw(14) << delta_pct
              << std::setw(14) << ratio
              << std::setw(14) << (r.series_ms / static_cast<double>(r.iters))
              << std::setw(14) << (r.torch_ms / static_cast<double>(r.iters))
              << "\n";
  }

  const int total_ops = static_cast<int>(rows.size());
  const double torch_level_score = (score_sum / static_cast<double>(total_ops)) * 100.0;
  std::cout << "\nTorch-level score (CPU, perf-only): " << std::fixed << std::setprecision(2)
            << torch_level_score << " / 100.00\n";
  std::cout << "Ops within 1.25x Torch: " << within_125 << "/" << total_ops << "\n";
  std::cout << "Ops within 1.50x Torch: " << within_150 << "/" << total_ops << "\n";
  std::cout << "Ops within 2.00x Torch: " << within_200 << "/" << total_ops << "\n";
  if (opts.emit_csv) {
    std::cout << "CSV,torch_level_score," << torch_level_score << "\n";
    std::cout << "CSV,ops_within_1.25x," << within_125 << "," << total_ops << "\n";
    std::cout << "CSV,ops_within_1.50x," << within_150 << "," << total_ops << "\n";
    std::cout << "CSV,ops_within_2.00x," << within_200 << "," << total_ops << "\n";
  }
  if (opts.enforce_target && torch_level_score < opts.target_score) {
    std::cerr << "Tranche gate failed: score " << torch_level_score
              << " < target " << opts.target_score << "\n";
    return 3;
  }

  if (std::isnan(static_cast<double>(sink))) {
    std::cerr << "sink is NaN\n";
  }

  return 0;
}
