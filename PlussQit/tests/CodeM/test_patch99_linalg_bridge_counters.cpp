#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("Linear algebra MPS bridge counters reset and track usage", "[linearalg][mps][bridge][counters]") {
  using namespace pluss::subsp::linearalg;

  linearalg_reset_dispatch_counters();
  auto c0 = linearalg_dispatch_counters();
  REQUIRE(c0.mps_bridge_total == 0);
  REQUIRE(c0.mps_bridge_det == 0);
  REQUIRE(c0.mps_bridge_inverse == 0);
  REQUIRE(c0.mps_bridge_triangular_solve == 0);

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  if (!linearalg_mps_cpu_bridge_enabled()) {
    SUCCEED("MPS linalg CPU bridge disabled by policy");
    return;
  }

  const auto a_cpu = pluss::make_series(
      std::vector<float>{
          3.0f, 1.0f, 0.0f,
          1.0f, 3.0f, 1.0f,
          0.0f, 1.0f, 2.0f},
      {3, 3});
  const auto b_cpu = pluss::make_series(std::vector<float>{1.0f, 0.0f, 1.0f}, {3, 1});
  const auto a_mps = a_cpu.to(pluss::pMPS);
  const auto b_mps = b_cpu.to(pluss::pMPS);

  const auto d = det(a_mps);
  const auto inv = inverse(a_mps);
  const auto tri = triangular_solve(cholesky(a_mps), b_mps);
  REQUIRE(d.platform() == pluss::pMPS);
  REQUIRE(inv.platform() == pluss::pMPS);
  REQUIRE(tri.platform() == pluss::pMPS);

  auto c1 = linearalg_dispatch_counters();
  REQUIRE(c1.mps_bridge_total == 0);
  REQUIRE(c1.mps_bridge_det == 0);
  REQUIRE(c1.mps_bridge_inverse == 0);
  REQUIRE(c1.mps_bridge_cholesky == 0);
  REQUIRE(c1.mps_bridge_triangular_solve == 0);

  linearalg_reset_dispatch_counters();
  auto c2 = linearalg_dispatch_counters();
  REQUIRE(c2.mps_bridge_total == 0);
  REQUIRE(c2.mps_bridge_det == 0);
  REQUIRE(c2.mps_bridge_inverse == 0);
  REQUIRE(c2.mps_bridge_cholesky == 0);
  REQUIRE(c2.mps_bridge_triangular_solve == 0);
}
