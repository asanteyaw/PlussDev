#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cmath>
#include <vector>

namespace {

void require_close_vec_(const std::vector<float>& got, const std::vector<float>& want, float atol = 1e-3f) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(std::fabs(got[i] - want[i]) <= atol);
  }
}

} // namespace

TEST_CASE("MPS native det family avoids bridge counters", "[mps][linearalg][native][det]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  if (!linearalg_mps_experimental_native_enabled()) {
    SUCCEED("Native det test requires PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE=1");
    return;
  }

  const auto a_cpu = pluss::make_series(
      std::vector<float>{
          3.5f, 1.0f, -0.4f,
          0.2f, 2.7f, 0.9f,
          -1.2f, 0.3f, 4.1f},
      {3, 3});
  const auto a_mps = a_cpu.to(pluss::pMPS);

  linearalg_reset_dispatch_counters();

  const auto det_ref = det(a_cpu).to(pluss::ScalarType::Float32).host<float>();
  const auto logdet_ref = logdet(a_cpu).to(pluss::ScalarType::Float32).host<float>();
  const auto slogdet_ref = slogdet(a_cpu).to(pluss::ScalarType::Float32).host<float>();

  const auto det_got = det(a_mps).to(pluss::pCPU).to(pluss::ScalarType::Float32).host<float>();
  const auto logdet_got = logdet(a_mps).to(pluss::pCPU).to(pluss::ScalarType::Float32).host<float>();
  const auto slogdet_got = slogdet(a_mps).to(pluss::pCPU).to(pluss::ScalarType::Float32).host<float>();

  require_close_vec_(det_got, det_ref, 3e-3f);
  require_close_vec_(logdet_got, logdet_ref, 3e-3f);
  require_close_vec_(slogdet_got, slogdet_ref, 3e-3f);

  const auto c = linearalg_dispatch_counters();
  REQUIRE(c.mps_bridge_total == 0);
  REQUIRE(c.mps_bridge_det == 0);
  REQUIRE(c.mps_bridge_logdet == 0);
  REQUIRE(c.mps_bridge_slogdet == 0);

  const auto singular_cpu = pluss::make_series(
      std::vector<float>{
          1.0f, 2.0f, 3.0f,
          1.0f, 2.0f, 3.0f,
          0.0f, 1.0f, 1.0f},
      {3, 3});
  const auto singular_mps = singular_cpu.to(pluss::pMPS);

  linearalg_reset_dispatch_counters();
  const float det_s = det(singular_mps).to(pluss::pCPU).to(pluss::ScalarType::Float32).item<float>();
  const float logdet_s = logdet(singular_mps).to(pluss::pCPU).to(pluss::ScalarType::Float32).item<float>();
  const auto slogdet_s = slogdet(singular_mps).to(pluss::pCPU).to(pluss::ScalarType::Float32).host<float>();

  REQUIRE(std::fabs(det_s) <= 1e-6f);
  REQUIRE(std::isinf(logdet_s));
  REQUIRE(logdet_s < 0.0f);
  REQUIRE(slogdet_s.size() == 2);
  REQUIRE(std::fabs(slogdet_s[0]) <= 1e-6f);
  REQUIRE(std::isinf(slogdet_s[1]));
  REQUIRE(slogdet_s[1] < 0.0f);

  const auto c2 = linearalg_dispatch_counters();
  REQUIRE(c2.mps_bridge_total == 0);
  REQUIRE(c2.mps_bridge_det == 0);
  REQUIRE(c2.mps_bridge_logdet == 0);
  REQUIRE(c2.mps_bridge_slogdet == 0);
}
