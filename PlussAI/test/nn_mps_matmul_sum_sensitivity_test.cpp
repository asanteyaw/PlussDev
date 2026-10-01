#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

bool approx_(double a, double b, double atol, double rtol) {
  return std::abs(a - b) <= (atol + rtol * std::abs(b));
}

bool run_case_(std::ostream* err = nullptr) {
  if (!pluss::mps::is_ready()) return true;

  const Config cpu = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const Config mps = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pMPS);
  const Config i64_mps = Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pMPS);

  {
    const Series base = pluss::make_series(std::vector<float>{1.f, 2.f, 3.f, 4.f}, {4}, mps);
    const Series device_written = base.mul(2.0).reshape({2, 2});
    const auto transposed = device_written.transpose(0, 1).to(pluss::pCPU, ScalarType::Float64).reshape({4}).to_vec<double>();
    const std::vector<double> expect_transposed{2.0, 6.0, 4.0, 8.0};
    for (std::size_t i = 0; i < expect_transposed.size(); ++i) {
      if (!approx_(transposed[i], expect_transposed[i], 1e-6, 1e-6)) {
        if (err) *err << "mps alias view forward sync mismatch\n";
        return false;
      }
    }

    const Series row = pluss::make_series(std::vector<float>{0.25f, 0.5f}, {2}, mps).add(1.0);
    const Series row_time = row.unsqueeze(0);
    const Series joined = row_time.unite(std::vector<Series>{row_time}, 0);
    const auto joined_v = joined.to(pluss::pCPU, ScalarType::Float64).reshape({4}).to_vec<double>();
    const std::vector<double> expect_joined{1.25, 1.5, 1.25, 1.5};
    for (std::size_t i = 0; i < expect_joined.size(); ++i) {
      if (!approx_(joined_v[i], expect_joined[i], 1e-6, 1e-6)) {
        if (err) *err << "mps alias unite forward sync mismatch\n";
        return false;
      }
    }
  }

  {
    Series v = pluss::make_series(std::vector<float>{1.f, 2.f, 3.f, 4.f}, {4}, mps);
    v.tracks_slope_(true);
    v.sum().reverse();
    if (!v.has_sensitivity()) {
      if (err) *err << "mps sum sensitivity missing\n";
      return false;
    }
    const auto gv = v.sensitivity().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    for (double g : gv) {
      if (!approx_(g, 1.0, 1e-5, 1e-5)) {
        if (err) *err << "mps sum sensitivity mismatch\n";
        return false;
      }
    }
  }

  {
    Series x = pluss::make_series(std::vector<float>{3.f, 4.f}, {1, 2}, mps);
    x.tracks_slope_(true);
    x.squeeze(0).sum().reverse();
    if (!x.has_sensitivity()) {
      if (err) *err << "mps squeeze sensitivity missing\n";
      return false;
    }
    const auto gx = x.sensitivity().to(pluss::pCPU, ScalarType::Float64).reshape({2}).to_vec<double>();
    if (!approx_(gx[0], 1.0, 1e-5, 1e-5) || !approx_(gx[1], 1.0, 1e-5, 1e-5)) {
      if (err) *err << "mps squeeze sensitivity mismatch\n";
      return false;
    }
  }

  {
    Series x = pluss::make_series(
        std::vector<float>{
            1.f, 2.f,
            3.f, 4.f,
            5.f, 6.f,
        },
        {3, 2},
        mps);
    x.tracks_slope_(true);
    const Series idx = pluss::make_series(std::vector<std::int64_t>{1}, {1}, i64_mps);
    const Series picked = x.index_select(0, idx);
    const auto picked_cpu = picked.to(pluss::pCPU, ScalarType::Float64).reshape({2}).to_vec<double>();
    if (!(approx_(picked_cpu[0], 3.0, 1e-6, 1e-6) && approx_(picked_cpu[1], 4.0, 1e-6, 1e-6))) {
      if (err) {
        *err << "mps index_select forward mismatch got=[";
        for (std::size_t j = 0; j < picked_cpu.size(); ++j) {
          *err << picked_cpu[j];
          if (j + 1 < picked_cpu.size()) *err << ",";
        }
        *err << "]\n";
      }
      return false;
    }
    x.clear();
    picked.sum().reverse();
    if (!x.has_sensitivity()) {
      if (err) *err << "mps index_select+sum sensitivity missing\n";
      return false;
    }
    {
      const auto gx = x.sensitivity().to(pluss::pCPU, ScalarType::Float64).reshape({6}).to_vec<double>();
      const std::vector<double> expect{0.0, 0.0, 1.0, 1.0, 0.0, 0.0};
      for (std::size_t i = 0; i < expect.size(); ++i) {
        if (!approx_(gx[i], expect[i], 1e-5, 1e-5)) {
          if (err) *err << "mps index_select+sum mismatch\n";
          return false;
        }
      }
    }
    x.clear();
    picked.squeeze(0).sum().reverse();
    if (!x.has_sensitivity()) {
      if (err) *err << "mps index_select+squeeze sensitivity missing\n";
      return false;
    }
    const auto gx = x.sensitivity().to(pluss::pCPU, ScalarType::Float64).reshape({6}).to_vec<double>();
    const std::vector<double> expect{0.0, 0.0, 1.0, 1.0, 0.0, 0.0};
    for (std::size_t i = 0; i < expect.size(); ++i) {
      if (!approx_(gx[i], expect[i], 1e-5, 1e-5)) {
        if (err) {
          *err << "mps index_select+squeeze mismatch";
          *err << " expected=[";
          for (std::size_t j = 0; j < expect.size(); ++j) {
            *err << expect[j];
            if (j + 1 < expect.size()) *err << ",";
          }
          *err << "] got=[";
          for (std::size_t j = 0; j < gx.size(); ++j) {
            *err << gx[j];
            if (j + 1 < gx.size()) *err << ",";
          }
          *err << "]\n";
        }
        return false;
      }
    }
  }

  {
    Series x = pluss::make_series(
        std::vector<float>{
            1.f, 2.f,
            3.f, 4.f,
        },
        {2, 2},
        mps);
    x.tracks_slope_(true);
    const Series sel = x.select(0, 1);
    sel.sum().reverse();
    if (!x.has_sensitivity()) {
      if (err) *err << "mps select sensitivity missing\n";
      return false;
    }
    const auto gx = x.sensitivity().to(pluss::pCPU, ScalarType::Float64).reshape({4}).to_vec<double>();
    const std::vector<double> expect{0.0, 0.0, 1.0, 1.0};
    for (std::size_t i = 0; i < expect.size(); ++i) {
      if (!approx_(gx[i], expect[i], 1e-5, 1e-5)) {
        if (err) {
          *err << "mps select sensitivity mismatch got=[";
          for (std::size_t j = 0; j < gx.size(); ++j) {
            *err << gx[j];
            if (j + 1 < gx.size()) *err << ",";
          }
          *err << "]\n";
        }
        return false;
      }
    }
  }

  {
    Series a = pluss::make_series(std::vector<float>{1.f, 2.f}, {2}, mps);
    Series b = pluss::make_series(std::vector<float>{3.f, 4.f}, {2}, mps);
    a.tracks_slope_(true);
    b.tracks_slope_(true);
    pluss::series::pile(std::vector<Series>{a, b}, 0).select(0, 1).sum().reverse();
    if (!a.has_sensitivity() || !b.has_sensitivity()) {
      if (err) *err << "mps pile/select sensitivity missing\n";
      return false;
    }
    const auto ga = a.sensitivity().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    const auto gb = b.sensitivity().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    for (double g : ga) {
      if (!approx_(g, 0.0, 1e-5, 1e-5)) {
        if (err) *err << "mps pile/select first branch mismatch\n";
        return false;
      }
    }
    for (double g : gb) {
      if (!approx_(g, 1.0, 1e-5, 1e-5)) {
        if (err) {
          *err << "mps pile/select second branch mismatch ga=[";
          for (std::size_t i = 0; i < ga.size(); ++i) {
            *err << ga[i];
            if (i + 1 < ga.size()) *err << ",";
          }
          *err << "] gb=[";
          for (std::size_t i = 0; i < gb.size(); ++i) {
            *err << gb[i];
            if (i + 1 < gb.size()) *err << ",";
          }
          *err << "]\n";
        }
        return false;
      }
    }
  }

  {
    const Config f32_mps = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pMPS);
    Series x = pluss::make_series(std::vector<float>{0.1f, -0.2f}, {2, 1}, f32_mps);
    Series h = pluss::make_series(std::vector<float>{0.05f, 0.10f}, {2, 1}, f32_mps);
    Series c = pluss::make_series(std::vector<float>{0.02f, -0.03f}, {2, 1}, f32_mps);
    Series wih = pluss::make_series(std::vector<float>{0.2f, -0.1f, 0.15f, 0.05f}, {4, 1}, f32_mps);
    Series whh = pluss::make_series(std::vector<float>{0.12f, -0.08f, 0.09f, 0.03f}, {4, 1}, f32_mps);
    Series bih = pluss::make_series(std::vector<float>{0.01f, 0.02f, -0.01f, 0.03f}, {4}, f32_mps);
    Series bhh = pluss::make_series(std::vector<float>{-0.02f, 0.01f, 0.02f, -0.01f}, {4}, f32_mps);

    wih.tracks_slope_(true);
    whh.tracks_slope_(true);
    bih.tracks_slope_(true);
    bhh.tracks_slope_(true);

    const Series gi = pluss::ai::ff(x, wih, bih);
    const Series gh = pluss::ai::ff(h, whh, bhh);
    const Series gates = gi.add(gh);
    const Series i = pluss::detail::nn::narrow_dim_(gates, 1, 0, 1).sigmoid();
    const Series f = pluss::detail::nn::narrow_dim_(gates, 1, 1, 1).sigmoid();
    const Series g = pluss::detail::nn::narrow_dim_(gates, 1, 2, 1).tanh();
    const Series o = pluss::detail::nn::narrow_dim_(gates, 1, 3, 1).sigmoid();
    const Series next_c = f.mul(c).add(i.mul(g));
    const Series next_h = o.mul(next_c.tanh());
    next_h.sum().reverse();

    auto grad_l1 = [](const Series& p) -> double {
      if (!p.has_sensitivity()) return 0.0;
      return p.sensitivity().abs().sum().to(pluss::pCPU, ScalarType::Float64).item<double>();
    };
    const double total = grad_l1(wih) + grad_l1(whh) + grad_l1(bih) + grad_l1(bhh);
    if (!(total > 0.0)) {
      if (err) *err << "mps lstm-cell parameter sensitivity missing\n";
      return false;
    }
  }

  {
    const Series x_cpu = pluss::make_series(
        std::vector<float>{
            -1.0f, 0.5f,
            -0.2f, 0.1f,
            0.3f, -0.4f,
            0.9f, -0.7f,
        },
        {4, 2},
        cpu);

    Series b_cpu = pluss::make_series(std::vector<float>{0.1f, -0.2f}, {2, 1}, cpu);
    b_cpu.tracks_slope_(true);
    x_cpu.matmul(b_cpu).sum().reverse();
    const auto gb_cpu = b_cpu.sensitivity().reshape({2}).to(pluss::pCPU, ScalarType::Float64).to_vec<double>();

    Series x_mps = x_cpu.to(pluss::pMPS, ScalarType::Float32);
    Series b_mps = b_cpu.detach().to(pluss::pMPS, ScalarType::Float32);
    b_mps.tracks_slope_(true);
    x_mps.matmul(b_mps).sum().reverse();
    if (!b_mps.has_sensitivity()) {
      if (err) *err << "mps matmul sensitivity missing\n";
      return false;
    }
    const auto gb_mps = b_mps.sensitivity().reshape({2}).to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    for (std::size_t i = 0; i < gb_cpu.size() && i < gb_mps.size(); ++i) {
      if (!approx_(gb_cpu[i], gb_mps[i], 1e-4, 1e-4)) {
        if (err) *err << "mps matmul parity mismatch\n";
        return false;
      }
    }
  }

  {
    const Series x_cpu = pluss::make_series(
        std::vector<float>{
            -1.0f, -1.25f, -1.20f,
            -0.9f, -1.05f, -1.00f,
            -0.8f, -0.95f, -0.90f,
            -0.7f, -0.85f, -0.80f,
        },
        {4, 3},
        cpu);
    const Series w_cpu = pluss::make_series(std::vector<float>{0.2f, -0.1f, 0.05f}, {3, 1}, cpu);
    const auto ref = x_cpu.matmul(w_cpu).to(pluss::pCPU, ScalarType::Float64).reshape({4}).to_vec<double>();

    const Series x_mps = x_cpu.to(pluss::pMPS, ScalarType::Float32);
    const Series w_mps = w_cpu.to(pluss::pMPS, ScalarType::Float32);
    const auto got = x_mps.matmul(w_mps).to(pluss::pCPU, ScalarType::Float64).reshape({4}).to_vec<double>();
    for (std::size_t i = 0; i < ref.size() && i < got.size(); ++i) {
      if (!approx_(ref[i], got[i], 1e-4, 1e-4)) {
        if (err) *err << "mps matmul (n=1,k=3) forward parity mismatch\n";
        return false;
      }
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("mps matmul+sum sensitivity parity") {
  REQUIRE(run_case_(&std::cerr));
}
#else
int main() {
  if (!run_case_(&std::cerr)) return 1;
  std::cout << "nn_mps_matmul_sum_sensitivity_test passed\n";
  return 0;
}
#endif
