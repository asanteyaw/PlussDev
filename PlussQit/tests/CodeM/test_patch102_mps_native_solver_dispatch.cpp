#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <vector>

TEST_CASE("MPS native solve-family avoids bridge counters", "[mps][linearalg][native][dispatch]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const auto l_cpu = pluss::make_series(
      std::vector<float>{
          2.1f, 0.0f, 0.0f,
          -0.5f, 1.8f, 0.0f,
          0.4f, 0.6f, 1.5f},
      {3, 3});
  const auto b_cpu = pluss::make_series(
      std::vector<float>{
          0.5f, 1.2f,
          -0.3f, 0.7f,
          1.5f, -0.4f},
      {3, 2});

  const auto a_cpu = l_cpu.matmul(l_cpu.transpose(0, 1));
  const auto factor_cpu = pluss::subsp::math::linalg::cholesky(a_cpu, false).to(pluss::ScalarType::Float32);
  const auto l_mps = factor_cpu.to(pluss::pMPS);
  const auto b_mps = b_cpu.to(pluss::pMPS);

  linearalg_reset_dispatch_counters();

  const auto chol = cholesky(a_cpu.to(pluss::pMPS), /*upper=*/false);
  const auto x_tri = triangular_solve(l_mps, b_mps, /*upper=*/false, /*unitriangular=*/false);
  const auto x_chol = cholesky_solve(l_mps, b_mps, /*upper=*/false);
  const auto inv = cholesky_inverse(l_mps, /*upper=*/false);

  REQUIRE(chol.platform() == pluss::pMPS);
  REQUIRE(x_tri.platform() == pluss::pMPS);
  REQUIRE(x_chol.platform() == pluss::pMPS);
  REQUIRE(inv.platform() == pluss::pMPS);

  const auto c = linearalg_dispatch_counters();
  REQUIRE(c.mps_bridge_total == 0);
  REQUIRE(c.mps_bridge_cholesky == 0);
  REQUIRE(c.mps_bridge_triangular_solve == 0);
  REQUIRE(c.mps_bridge_cholesky_solve == 0);
  REQUIRE(c.mps_bridge_cholesky_inverse == 0);
}
