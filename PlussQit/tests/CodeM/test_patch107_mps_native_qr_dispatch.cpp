#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

namespace {

float max_abs_diff_(const pluss::Series& a, const pluss::Series& b) {
  return a.sub(b).abs().amax().item<float>();
}

pluss::Series make_full_rank_matrix_(
    std::int64_t m,
    std::int64_t n,
    std::mt19937& rng) {
  std::uniform_real_distribution<float> dist(-0.8f, 0.8f);
  std::vector<float> data(static_cast<std::size_t>(m * n), 0.0f);
  for (auto& v : data) {
    v = dist(rng);
  }
  const auto d = std::min(m, n);
  for (std::int64_t i = 0; i < d; ++i) {
    data[static_cast<std::size_t>(i * n + i)] += 2.0f;
  }
  return pluss::make_series(data, {m, n});
}

} // namespace

TEST_CASE("MPS native qr avoids bridge counters on deterministic cases", "[mps][linearalg][native][qr]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  if (!linearalg_mps_experimental_native_enabled()) {
    SUCCEED("Native QR test requires PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE=1");
    return;
  }

  const auto a_cpu = pluss::make_series(
      std::vector<float>{
          2.3f, -0.2f, 0.4f,
          0.1f, 2.8f, -0.6f,
          -0.4f, 0.7f, 2.6f,
          0.8f, -0.3f, 0.5f,
          -0.2f, 0.4f, 1.8f},
      {5, 3});
  const auto a_mps = a_cpu.to(pluss::pMPS);

  linearalg_reset_dispatch_counters();
  const auto q_mps = qr_q(a_mps, true);
  const auto r_mps = qr_r(a_mps, true);

  REQUIRE(q_mps.platform() == pluss::pMPS);
  REQUIRE(r_mps.platform() == pluss::pMPS);

  const auto recon = q_mps.matmul(r_mps).to(pluss::pCPU).to(pluss::ScalarType::Float32);
  REQUIRE(max_abs_diff_(recon, a_cpu.to(pluss::ScalarType::Float32)) < 1.5e-2f);

  const auto qtq = q_mps.transpose(0, 1).matmul(q_mps).to(pluss::pCPU).to(pluss::ScalarType::Float32);
  const auto eye = pluss::series::eye(q_mps.sizes()[1], q_mps.sizes()[1], pluss::ScalarType::Float32);
  REQUIRE(max_abs_diff_(qtq, eye) < 2.0e-2f);

  const auto c = linearalg_dispatch_counters();
  REQUIRE(c.mps_bridge_total == 0);
  REQUIRE(c.mps_bridge_qr == 0);
}

TEST_CASE("MPS native qr stress/repeat stability", "[mps][linearalg][native][qr][stress]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  if (!linearalg_mps_experimental_native_enabled()) {
    SUCCEED("Native QR stress test requires PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE=1");
    return;
  }

  std::mt19937 rng(20260325u);
  std::uniform_int_distribution<std::int64_t> dim_dist(2, 7);

  linearalg_reset_dispatch_counters();

  for (std::int64_t iter = 0; iter < 120; ++iter) {
    const std::int64_t m = dim_dist(rng);
    const std::int64_t n = dim_dist(rng);
    const auto a_cpu = make_full_rank_matrix_(m, n, rng);
    const auto a_mps = a_cpu.to(pluss::pMPS);

    const auto q = qr_q(a_mps, true);
    const auto r = qr_r(a_mps, true);

    const auto recon = q.matmul(r).to(pluss::pCPU).to(pluss::ScalarType::Float32);
    REQUIRE(max_abs_diff_(recon, a_cpu.to(pluss::ScalarType::Float32)) < 2.5e-2f);

    const auto qtq = q.transpose(0, 1).matmul(q).to(pluss::pCPU).to(pluss::ScalarType::Float32);
    const auto eye = pluss::series::eye(q.sizes()[1], q.sizes()[1], pluss::ScalarType::Float32);
    REQUIRE(max_abs_diff_(qtq, eye) < 3.5e-2f);
  }

  const auto stable_cpu = pluss::make_series(
      std::vector<float>{
          2.7f, 0.1f, -0.2f, 0.5f,
          -0.2f, 3.1f, 0.4f, -0.6f,
          0.7f, -0.5f, 2.9f, 0.2f,
          0.4f, 0.3f, -0.1f, 2.6f},
      {4, 4});
  const auto stable_mps = stable_cpu.to(pluss::pMPS);
  const auto a_ref = stable_cpu.to(pluss::ScalarType::Float32);
  const auto eye4 = pluss::series::eye(4, 4, pluss::ScalarType::Float32);

  for (std::int64_t rep = 0; rep < 220; ++rep) {
    const auto q = qr_q(stable_mps, true);
    const auto r = qr_r(stable_mps, true);
    const auto recon = q.matmul(r).to(pluss::pCPU).to(pluss::ScalarType::Float32);
    const auto qtq = q.transpose(0, 1).matmul(q).to(pluss::pCPU).to(pluss::ScalarType::Float32);
    REQUIRE(max_abs_diff_(recon, a_ref) < 2.0e-2f);
    REQUIRE(max_abs_diff_(qtq, eye4) < 3.0e-2f);
  }

  const auto c = linearalg_dispatch_counters();
  REQUIRE(c.mps_bridge_total == 0);
  REQUIRE(c.mps_bridge_qr == 0);
}
