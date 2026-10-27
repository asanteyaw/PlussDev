#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

namespace {

void require_close_vec_(const std::vector<float>& got, const std::vector<float>& want, float atol = 1e-3f) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(std::fabs(got[i] - want[i]) <= atol);
  }
}

} // namespace

TEST_CASE("MPS linalg bridge deterministic parity", "[mps][linearalg][bridge]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  if (!linearalg_mps_cpu_bridge_enabled()) {
    SUCCEED("MPS linalg CPU bridge disabled by policy");
    return;
  }
  if (linearalg_mps_experimental_native_enabled()) {
    SUCCEED("Bridge parity test expects PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE=0");
    return;
  }

  const auto a_cpu = pluss::make_series(
      std::vector<float>{
          4.0f, 1.0f, 1.0f,
          1.0f, 3.0f, 0.0f,
          1.0f, 0.0f, 2.0f},
      {3, 3});
  const auto b_cpu = pluss::make_series(std::vector<float>{1.0f, 2.0f, 3.0f}, {3, 1});

  const auto a_mps = a_cpu.to(pluss::pMPS);
  const auto b_mps = b_cpu.to(pluss::pMPS);

  const auto det_ref = pluss::subsp::linearalg::det(a_cpu).to(pluss::ScalarType::Float32);
  const auto det_got = pluss::subsp::linearalg::det(a_mps).to(pluss::pCPU).to(pluss::ScalarType::Float32);
  require_close_vec_(det_got.host<float>().array().data, det_ref.host<float>().array().data, 1e-4f);

  const auto logdet_ref = pluss::subsp::linearalg::logdet(a_cpu).to(pluss::ScalarType::Float32);
  const auto logdet_got = pluss::subsp::linearalg::logdet(a_mps).to(pluss::pCPU).to(pluss::ScalarType::Float32);
  require_close_vec_(logdet_got.host<float>().array().data, logdet_ref.host<float>().array().data, 1e-4f);

  const auto slogdet_ref = pluss::subsp::linearalg::slogdet(a_cpu).to(pluss::ScalarType::Float32);
  const auto slogdet_got = pluss::subsp::linearalg::slogdet(a_mps).to(pluss::pCPU).to(pluss::ScalarType::Float32);
  require_close_vec_(slogdet_got.host<float>().array().data, slogdet_ref.host<float>().array().data, 1e-4f);

  const auto inv_ref = pluss::subsp::linearalg::inverse(a_cpu).to(pluss::ScalarType::Float32);
  const auto inv_got = pluss::subsp::linearalg::inverse(a_mps).to(pluss::pCPU).to(pluss::ScalarType::Float32);
  require_close_vec_(inv_got.host<float>().array().data, inv_ref.host<float>().array().data, 2e-3f);

  const auto chol_ref = pluss::subsp::linearalg::cholesky(a_cpu, false).to(pluss::ScalarType::Float32);
  const auto chol_mps = pluss::subsp::linearalg::cholesky(a_mps, false);
  const auto chol_got = chol_mps.to(pluss::pCPU).to(pluss::ScalarType::Float32);
  require_close_vec_(chol_got.host<float>().array().data, chol_ref.host<float>().array().data, 2e-3f);

  const auto tri_ref = pluss::subsp::linearalg::triangular_solve(chol_ref, b_cpu, false, false).to(pluss::ScalarType::Float32);
  const auto tri_got =
      pluss::subsp::linearalg::triangular_solve(chol_mps, b_mps, false, false).to(pluss::pCPU).to(pluss::ScalarType::Float32);
  require_close_vec_(tri_got.host<float>().array().data, tri_ref.host<float>().array().data, 2e-3f);

  const auto chol_solve_ref = pluss::subsp::linearalg::cholesky_solve(chol_ref, b_cpu, false).to(pluss::ScalarType::Float32);
  const auto chol_solve_got =
      pluss::subsp::linearalg::cholesky_solve(chol_mps, b_mps, false).to(pluss::pCPU).to(pluss::ScalarType::Float32);
  require_close_vec_(chol_solve_got.host<float>().array().data, chol_solve_ref.host<float>().array().data, 2e-3f);

  const auto chol_inv_ref = pluss::subsp::linearalg::cholesky_inverse(chol_ref, false).to(pluss::ScalarType::Float32);
  const auto chol_inv_got =
      pluss::subsp::linearalg::cholesky_inverse(chol_mps, false).to(pluss::pCPU).to(pluss::ScalarType::Float32);
  require_close_vec_(chol_inv_got.host<float>().array().data, chol_inv_ref.host<float>().array().data, 2e-3f);

  const auto lu_ref = pluss::subsp::linearalg::lu_solve(a_cpu, b_cpu).to(pluss::ScalarType::Float32);
  const auto lu_got = pluss::subsp::linearalg::lu_solve(a_mps, b_mps).to(pluss::pCPU).to(pluss::ScalarType::Float32);
  require_close_vec_(lu_got.host<float>().array().data, lu_ref.host<float>().array().data, 2e-3f);

  const auto tall_cpu = pluss::make_series(
      std::vector<float>{
          1.0f, 2.0f, 3.0f,
          0.0f, 1.0f, 4.0f,
          2.0f, 0.0f, 1.0f,
          1.0f, 1.0f, 0.0f},
      {4, 3});
  const auto pinv_ref = pluss::subsp::linearalg::pinverse(tall_cpu, 1e-8).to(pluss::ScalarType::Float32);
  const auto pinv_got =
      pluss::subsp::linearalg::pinverse(tall_cpu.to(pluss::pMPS), 1e-8).to(pluss::pCPU).to(pluss::ScalarType::Float32);
  require_close_vec_(pinv_got.host<float>().array().data, pinv_ref.host<float>().array().data, 3e-3f);
}

TEST_CASE("MPS linalg bridge stress and stability", "[mps][linearalg][bridge][stress]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  if (!linearalg_mps_cpu_bridge_enabled()) {
    SUCCEED("MPS linalg CPU bridge disabled by policy");
    return;
  }
  if (linearalg_mps_experimental_native_enabled()) {
    SUCCEED("Bridge stress test expects PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE=0");
    return;
  }

  std::mt19937 rng(20260316u);
  std::uniform_int_distribution<std::int64_t> n_dist(2, 5);
  std::uniform_real_distribution<float> val_dist(-1.0f, 1.0f);

  for (std::int64_t iter = 0; iter < 90; ++iter) {
    const auto n = n_dist(rng);
    std::vector<float> m(static_cast<std::size_t>(n * n), 0.0f);
    for (auto& v : m) {
      v = val_dist(rng);
    }

    std::vector<float> a(static_cast<std::size_t>(n * n), 0.0f);
    for (std::int64_t i = 0; i < n; ++i) {
      for (std::int64_t j = 0; j < n; ++j) {
        float acc = 0.0f;
        for (std::int64_t k = 0; k < n; ++k) {
          acc += m[static_cast<std::size_t>(k * n + i)] * m[static_cast<std::size_t>(k * n + j)];
        }
        if (i == j) {
          acc += static_cast<float>(n);
        }
        a[static_cast<std::size_t>(i * n + j)] = acc;
      }
    }

    std::vector<float> rhs(static_cast<std::size_t>(n * 2), 0.0f);
    for (auto& v : rhs) {
      v = val_dist(rng);
    }

    const auto a_cpu = pluss::make_series(a, {n, n});
    const auto b_cpu = pluss::make_series(rhs, {n, 2});
    const auto a_mps = a_cpu.to(pluss::pMPS);
    const auto b_mps = b_cpu.to(pluss::pMPS);

    const auto inv_ref = pluss::subsp::linearalg::inverse(a_cpu).to(pluss::ScalarType::Float32);
    const auto inv_got = pluss::subsp::linearalg::inverse(a_mps).to(pluss::pCPU).to(pluss::ScalarType::Float32);
    require_close_vec_(inv_got.host<float>().array().data, inv_ref.host<float>().array().data, 4e-3f);

    const auto sol_ref =
        pluss::subsp::linearalg::cholesky_solve(pluss::subsp::linearalg::cholesky(a_cpu), b_cpu).to(pluss::ScalarType::Float32);
    const auto sol_got =
        pluss::subsp::linearalg::cholesky_solve(pluss::subsp::linearalg::cholesky(a_mps), b_mps)
            .to(pluss::pCPU)
            .to(pluss::ScalarType::Float32);
    require_close_vec_(sol_got.host<float>().array().data, sol_ref.host<float>().array().data, 4e-3f);
  }

  const auto stable_cpu = pluss::make_series(
      std::vector<float>{
          5.0f, 1.0f, 0.0f,
          1.0f, 4.0f, 1.0f,
          0.0f, 1.0f, 3.0f},
      {3, 3});
  const auto stable_mps = stable_cpu.to(pluss::pMPS);
  const auto inv_ref = pluss::subsp::linearalg::inverse(stable_cpu).to(pluss::ScalarType::Float32).host<float>().array().data;

  for (std::int64_t rep = 0; rep < 240; ++rep) {
    const auto got =
        pluss::subsp::linearalg::inverse(stable_mps).to(pluss::pCPU).to(pluss::ScalarType::Float32).host<float>().array().data;
    require_close_vec_(got, inv_ref, 2e-3f);
  }
}
