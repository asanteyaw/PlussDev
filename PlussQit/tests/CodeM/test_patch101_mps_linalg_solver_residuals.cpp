#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

namespace {

std::vector<float> to_vec_f32_cpu_(const pluss::Series& x) {
  return x.to(pluss::pCPU).to(pluss::ScalarType::Float32).contiguous().to_vec<float>();
}

float max_abs_diff_(const std::vector<float>& a, const std::vector<float>& b) {
  REQUIRE(a.size() == b.size());
  float out = 0.0f;
  for (std::size_t i = 0; i < a.size(); ++i) {
    out = std::max(out, std::fabs(a[i] - b[i]));
  }
  return out;
}

std::vector<float> matmul_row_major_(
    const std::vector<float>& a, std::int64_t m, std::int64_t k,
    const std::vector<float>& b, std::int64_t n) {
  std::vector<float> out(static_cast<std::size_t>(m * n), 0.0f);
  for (std::int64_t i = 0; i < m; ++i) {
    for (std::int64_t p = 0; p < k; ++p) {
      const float av = a[static_cast<std::size_t>(i * k + p)];
      const std::size_t bbase = static_cast<std::size_t>(p * n);
      const std::size_t obase = static_cast<std::size_t>(i * n);
      for (std::int64_t j = 0; j < n; ++j) {
        out[obase + static_cast<std::size_t>(j)] += av * b[bbase + static_cast<std::size_t>(j)];
      }
    }
  }
  return out;
}

std::vector<float> transpose_row_major_(const std::vector<float>& a, std::int64_t m, std::int64_t n) {
  std::vector<float> out(static_cast<std::size_t>(m * n), 0.0f);
  for (std::int64_t r = 0; r < m; ++r) {
    for (std::int64_t c = 0; c < n; ++c) {
      out[static_cast<std::size_t>(c * m + r)] = a[static_cast<std::size_t>(r * n + c)];
    }
  }
  return out;
}

std::vector<float> make_spd_(const std::vector<float>& base, std::int64_t n) {
  const auto bt = transpose_row_major_(base, n, n);
  auto a = matmul_row_major_(bt, n, n, base, n);
  for (std::int64_t i = 0; i < n; ++i) {
    a[static_cast<std::size_t>(i * n + i)] += static_cast<float>(n) + 0.5f;
  }
  return a;
}

void require_finite_(const std::vector<float>& v) {
  for (const float x : v) {
    REQUIRE(std::isfinite(x));
  }
}

} // namespace

TEST_CASE("MPS linalg solver bridge deterministic residual checks", "[mps][linearalg][bridge][solver][residual]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  if (!linearalg_mps_cpu_bridge_enabled()) {
    SUCCEED("MPS linalg CPU bridge disabled by policy");
    return;
  }

  const auto l_cpu = pluss::make_series(
      std::vector<float>{
          2.0f, 0.0f, 0.0f, 0.0f,
          -1.0f, 2.5f, 0.0f, 0.0f,
          0.5f, 1.0f, 1.8f, 0.0f,
          -0.4f, 0.3f, 0.6f, 1.6f},
      {4, 4});
  const auto b_cpu = pluss::make_series(
      std::vector<float>{
          1.0f, -0.3f,
          0.4f, 1.2f,
          -0.8f, 0.7f,
          0.6f, -1.1f},
      {4, 2});
  const auto l_mps = l_cpu.to(pluss::pMPS);
  const auto b_mps = b_cpu.to(pluss::pMPS);

  const auto x_tri = triangular_solve(l_mps, b_mps, /*upper=*/false, /*unitriangular=*/false);
  REQUIRE(x_tri.platform() == pluss::pMPS);
  const auto tri_recon = to_vec_f32_cpu_(l_cpu.matmul(x_tri.to(pluss::pCPU)));
  REQUIRE(max_abs_diff_(tri_recon, to_vec_f32_cpu_(b_cpu)) <= 8e-3f);

  const auto a_cpu = l_cpu.matmul(l_cpu.transpose(0, 1));
  const auto a_mps = a_cpu.to(pluss::pMPS);
  const auto chol = cholesky(a_mps, false);
  const auto x_chol = cholesky_solve(chol, b_mps, false);
  REQUIRE(x_chol.platform() == pluss::pMPS);
  const auto chol_recon = to_vec_f32_cpu_(a_cpu.matmul(x_chol.to(pluss::pCPU)));
  REQUIRE(max_abs_diff_(chol_recon, to_vec_f32_cpu_(b_cpu)) <= 1e-2f);

  const auto inv = cholesky_inverse(chol, false);
  REQUIRE(inv.platform() == pluss::pMPS);
  const auto eye = pluss::Series::eye(4, 4, pluss::ScalarType::Float32);
  const auto prod = to_vec_f32_cpu_(a_cpu.to(pluss::ScalarType::Float32).matmul(inv.to(pluss::pCPU)));
  REQUIRE(max_abs_diff_(prod, to_vec_f32_cpu_(eye)) <= 1.5e-2f);

  const auto x_lu = lu_solve(a_mps, b_mps);
  REQUIRE(x_lu.platform() == pluss::pMPS);
  const auto lu_recon = to_vec_f32_cpu_(a_cpu.matmul(x_lu.to(pluss::pCPU)));
  REQUIRE(max_abs_diff_(lu_recon, to_vec_f32_cpu_(b_cpu)) <= 1.5e-2f);
}

TEST_CASE("MPS linalg solver bridge stress stability residuals", "[mps][linearalg][bridge][solver][stress]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  if (!linearalg_mps_cpu_bridge_enabled()) {
    SUCCEED("MPS linalg CPU bridge disabled by policy");
    return;
  }

  std::mt19937 rng(20260323u);
  std::uniform_int_distribution<std::int64_t> n_dist(2, 6);
  std::uniform_int_distribution<std::int64_t> rhs_dist(1, 3);
  std::uniform_real_distribution<float> val_dist(-1.0f, 1.0f);

  for (std::int64_t iter = 0; iter < 110; ++iter) {
    const auto n = n_dist(rng);
    const auto nrhs = rhs_dist(rng);

    std::vector<float> base(static_cast<std::size_t>(n * n), 0.0f);
    for (auto& v : base) v = val_dist(rng);
    const auto a = make_spd_(base, n);

    std::vector<float> b(static_cast<std::size_t>(n * nrhs), 0.0f);
    for (auto& v : b) v = val_dist(rng);

    const auto a_cpu = pluss::make_series(a, {n, n});
    const auto b_cpu = pluss::make_series(b, {n, nrhs});
    const auto a_mps = a_cpu.to(pluss::pMPS);
    const auto b_mps = b_cpu.to(pluss::pMPS);

    const auto chol = cholesky(a_mps, false);
    const auto x_chol = cholesky_solve(chol, b_mps, false);
    const auto chol_recon = to_vec_f32_cpu_(a_cpu.matmul(x_chol.to(pluss::pCPU)));
    REQUIRE(max_abs_diff_(chol_recon, to_vec_f32_cpu_(b_cpu)) <= 3e-2f);
    require_finite_(to_vec_f32_cpu_(x_chol));

    const auto inv = cholesky_inverse(chol, false);
    const auto eye = pluss::Series::eye(n, n, pluss::ScalarType::Float32);
    const auto prod = to_vec_f32_cpu_(a_cpu.to(pluss::ScalarType::Float32).matmul(inv.to(pluss::pCPU)));
    REQUIRE(max_abs_diff_(prod, to_vec_f32_cpu_(eye)) <= 3.5e-2f);
    require_finite_(to_vec_f32_cpu_(inv));

    const auto x_lu = lu_solve(a_mps, b_mps);
    const auto lu_recon = to_vec_f32_cpu_(a_cpu.matmul(x_lu.to(pluss::pCPU)));
    REQUIRE(max_abs_diff_(lu_recon, to_vec_f32_cpu_(b_cpu)) <= 3e-2f);
    require_finite_(to_vec_f32_cpu_(x_lu));

    const auto l_cpu = cholesky(a_cpu, false).to(pluss::ScalarType::Float32);
    const auto x_tri = triangular_solve(l_cpu.to(pluss::pMPS), b_mps, false, false);
    const auto tri_recon = to_vec_f32_cpu_(l_cpu.matmul(x_tri.to(pluss::pCPU)));
    REQUIRE(max_abs_diff_(tri_recon, to_vec_f32_cpu_(b_cpu)) <= 3e-2f);
    require_finite_(to_vec_f32_cpu_(x_tri));
  }
}
