#include <pluss/pluss.h>
#include <pluss/codem/torch_interop.h>

#include <torch/torch.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
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

} // namespace

int main() {
  torch::manual_seed(42);

  constexpr std::int64_t N = 10'000'000;
  constexpr int iters_bin = 30;
  constexpr int iters_unary = 20;
  constexpr int iters_index = 10;

  auto a_t = torch::full({N}, 1.25f, torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU));
  auto b_t = torch::full({N}, 2.00f, torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU));

  auto a = pluss::codem::from_torch(a_t);
  auto b = pluss::codem::from_torch(b_t);

  volatile float sink = 0.0f;

  // Warmup
  auto c = a.add(b);
  c = c.mul(a);
  c = c.exp();
  auto c_t = a_t + b_t;
  c_t = c_t * a_t;
  c_t = torch::exp(c_t);

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

  std::cout << std::left << std::setw(10) << "op"
            << std::right << std::setw(14) << "Series ms"
            << std::setw(14) << "Torch ms"
            << std::setw(14) << "delta %"
            << std::setw(14) << "S/T"
            << std::setw(14) << "S iter ms"
            << std::setw(14) << "T iter ms"
            << "\n";

  for (const auto& r : rows) {
    const double delta_pct = (r.series_ms / r.torch_ms - 1.0) * 100.0;
    const double ratio = r.series_ms / r.torch_ms;
    std::cout << std::left << std::setw(10) << r.op
              << std::right << std::setw(14) << std::fixed << std::setprecision(3) << r.series_ms
              << std::setw(14) << r.torch_ms
              << std::setw(14) << delta_pct
              << std::setw(14) << ratio
              << std::setw(14) << (r.series_ms / static_cast<double>(r.iters))
              << std::setw(14) << (r.torch_ms / static_cast<double>(r.iters))
              << "\n";
  }

  if (std::isnan(static_cast<double>(sink))) {
    std::cerr << "sink is NaN\n";
  }

  return 0;
}
