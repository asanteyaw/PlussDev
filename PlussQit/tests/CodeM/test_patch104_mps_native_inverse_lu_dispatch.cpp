#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <vector>

TEST_CASE("MPS native inverse and lu_solve avoid bridge counters", "[mps][linearalg][native][inverse][lu]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  if (!linearalg_mps_experimental_native_enabled()) {
    SUCCEED("Native inverse/lu test requires PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE=1");
    return;
  }

  const auto a_cpu = pluss::make_series(
      std::vector<float>{
          4.0f, -1.0f, 0.5f,
          1.0f, 3.0f, -0.7f,
          -0.2f, 0.6f, 2.5f},
      {3, 3});
  const auto b_cpu = pluss::make_series(
      std::vector<float>{
          1.0f, -0.2f,
          0.4f, 1.5f,
          -1.1f, 0.8f},
      {3, 2});

  const auto a_mps = a_cpu.to(pluss::pMPS);
  const auto b_mps = b_cpu.to(pluss::pMPS);

  linearalg_reset_dispatch_counters();
  const auto inv_mps = inverse(a_mps);
  const auto x_mps = lu_solve(a_mps, b_mps);

  REQUIRE(inv_mps.platform() == pluss::pMPS);
  REQUIRE(x_mps.platform() == pluss::pMPS);

  const auto c = linearalg_dispatch_counters();
  REQUIRE(c.mps_bridge_total == 0);
  REQUIRE(c.mps_bridge_inverse == 0);
  REQUIRE(c.mps_bridge_lu_solve == 0);

  const auto inv_ref = pluss::subsp::math::linalg::inverse(a_cpu).to(pluss::ScalarType::Float32);
  const auto x_ref = pluss::subsp::math::linalg::lu_solve(b_cpu, a_cpu).to(pluss::ScalarType::Float32);

  const float inv_err = inv_mps.to(pluss::pCPU)
                            .to(pluss::ScalarType::Float32)
                            .sub(inv_ref)
                            .abs()
                            .amax()
                            .item<float>();
  const float lu_err = x_mps.to(pluss::pCPU)
                           .to(pluss::ScalarType::Float32)
                           .sub(x_ref)
                           .abs()
                           .amax()
                           .item<float>();
  const float residual = a_mps.matmul(x_mps)
                             .to(pluss::pCPU)
                             .to(pluss::ScalarType::Float32)
                             .sub(b_cpu.to(pluss::ScalarType::Float32))
                             .abs()
                             .amax()
                             .item<float>();

  REQUIRE(inv_err < 4e-3f);
  REQUIRE(lu_err < 4e-3f);
  REQUIRE(residual < 1e-2f);
}
