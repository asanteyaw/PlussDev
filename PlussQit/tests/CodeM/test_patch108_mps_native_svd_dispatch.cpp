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
  std::uniform_real_distribution<float> dist(-0.7f, 0.7f);
  std::vector<float> data(static_cast<std::size_t>(m * n), 0.0f);
  for (auto& v : data) {
    v = dist(rng);
  }
  const auto d = std::min(m, n);
  for (std::int64_t i = 0; i < d; ++i) {
    data[static_cast<std::size_t>(i * n + i)] += 2.3f;
  }
  return pluss::make_series(data, {m, n});
}

pluss::Series svd_reconstruct_(const pluss::Series& u, const pluss::Series& s, const pluss::Series& vh) {
  auto us = u.to(pluss::pCPU).to(pluss::ScalarType::Float32).contiguous();
  const auto ss = s.to(pluss::pCPU).to(pluss::ScalarType::Float32).contiguous().to_vec<float>();
  const auto us_sizes = us.sizes();
  const auto m = us_sizes[0];
  const auto k = us_sizes[1];
  auto uv = us.to_vec<float>();
  for (std::int64_t i = 0; i < m; ++i) {
    for (std::int64_t j = 0; j < k; ++j) {
      uv[static_cast<std::size_t>(i * k + j)] *= ss[static_cast<std::size_t>(j)];
    }
  }
  const auto us_scaled = pluss::make_series(uv, {m, k}).to(pluss::pCPU).to(pluss::ScalarType::Float32);
  return us_scaled.matmul(vh.to(pluss::pCPU).to(pluss::ScalarType::Float32));
}

} // namespace

TEST_CASE("MPS native svd avoids bridge counters on deterministic cases", "[mps][linearalg][native][svd]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const auto a_cpu = pluss::make_series(
      std::vector<float>{
          2.7f, -0.2f, 0.5f,
          0.2f, 3.1f, -0.4f,
          -0.3f, 0.5f, 2.4f,
          0.7f, -0.2f, 0.8f},
      {4, 3});
  const auto a_mps = a_cpu.to(pluss::pMPS);

  linearalg_reset_dispatch_counters();
  const auto u = svd_u(a_mps, false);
  const auto s = svd_s(a_mps, false);
  const auto vh = svd_vh(a_mps, false);

  REQUIRE(u.platform() == pluss::pMPS);
  REQUIRE(s.platform() == pluss::pMPS);
  REQUIRE(vh.platform() == pluss::pMPS);

  const auto recon = svd_reconstruct_(u, s, vh).to(pluss::ScalarType::Float32);
  REQUIRE(max_abs_diff_(recon, a_cpu.to(pluss::ScalarType::Float32)) < 5e-2f);

  const auto c = linearalg_dispatch_counters();
  REQUIRE(c.mps_bridge_total == 0);
  REQUIRE(c.mps_bridge_svd == 0);
}

TEST_CASE("MPS native svd stress/repeat stability", "[mps][linearalg][native][svd][stress]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  std::mt19937 rng(20260326u);
  std::uniform_int_distribution<std::int64_t> dim_dist(2, 6);

  linearalg_reset_dispatch_counters();

  for (std::int64_t iter = 0; iter < 100; ++iter) {
    const auto m = dim_dist(rng);
    const auto n = dim_dist(rng);
    const auto a_cpu = make_full_rank_matrix_(m, n, rng);
    const auto a_mps = a_cpu.to(pluss::pMPS);

    const auto u = svd_u(a_mps, false);
    const auto s = svd_s(a_mps, false);
    const auto vh = svd_vh(a_mps, false);
    const auto recon = svd_reconstruct_(u, s, vh).to(pluss::ScalarType::Float32);
    REQUIRE(max_abs_diff_(recon, a_cpu.to(pluss::ScalarType::Float32)) < 6e-2f);

    const auto s_vec = s.to(pluss::pCPU).to(pluss::ScalarType::Float32).to_vec<float>();
    for (const auto v : s_vec) {
      REQUIRE(v >= -1e-6f);
    }
  }

  const auto stable_cpu = pluss::make_series(
      std::vector<float>{
          3.0f, -0.2f, 0.4f, 0.1f,
          0.3f, 2.6f, -0.5f, 0.2f,
          -0.4f, 0.7f, 2.9f, -0.3f},
      {3, 4});
  const auto stable_mps = stable_cpu.to(pluss::pMPS);
  const auto ref = stable_cpu.to(pluss::ScalarType::Float32);

  for (std::int64_t rep = 0; rep < 180; ++rep) {
    const auto u = svd_u(stable_mps, false);
    const auto s = svd_s(stable_mps, false);
    const auto vh = svd_vh(stable_mps, false);
    const auto recon = svd_reconstruct_(u, s, vh).to(pluss::ScalarType::Float32);
    REQUIRE(max_abs_diff_(recon, ref) < 6e-2f);
  }

  const auto c = linearalg_dispatch_counters();
  REQUIRE(c.mps_bridge_total == 0);
  REQUIRE(c.mps_bridge_svd == 0);
}
