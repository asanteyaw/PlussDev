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
    data[static_cast<std::size_t>(i * n + i)] += 2.5f;
  }
  return pluss::make_series(data, {m, n});
}

} // namespace

TEST_CASE("MPS native pinverse avoids bridge counters on deterministic cases", "[mps][linearalg][native][pinverse]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const auto tall_cpu = pluss::make_series(
      std::vector<float>{
          3.1f, -0.2f, 0.4f,
          0.5f, 2.9f, -0.6f,
          -0.4f, 0.7f, 3.4f,
          0.8f, -0.3f, 0.5f,
          -0.2f, 0.4f, 1.8f},
      {5, 3});
  const auto wide_cpu = pluss::make_series(
      std::vector<float>{
          2.8f, -0.1f, 0.3f, 0.6f, -0.2f,
          0.2f, 3.2f, -0.5f, 0.1f, 0.4f,
          -0.6f, 0.3f, 2.5f, -0.4f, 0.9f},
      {3, 5});

  const auto tall_mps = tall_cpu.to(pluss::pMPS);
  const auto wide_mps = wide_cpu.to(pluss::pMPS);

  linearalg_reset_dispatch_counters();

  const auto tall_ref = pinverse(tall_cpu, 1e-6).to(pluss::ScalarType::Float32);
  const auto wide_ref = pinverse(wide_cpu, 1e-6).to(pluss::ScalarType::Float32);
  const auto tall_got = pinverse(tall_mps, 1e-6).to(pluss::pCPU).to(pluss::ScalarType::Float32);
  const auto wide_got = pinverse(wide_mps, 1e-6).to(pluss::pCPU).to(pluss::ScalarType::Float32);

  REQUIRE(max_abs_diff_(tall_got, tall_ref) < 1.2e-2f);
  REQUIRE(max_abs_diff_(wide_got, wide_ref) < 1.2e-2f);

  const auto c = linearalg_dispatch_counters();
  REQUIRE(c.mps_bridge_total == 0);
  REQUIRE(c.mps_bridge_pinverse == 0);
}

TEST_CASE("MPS native pinverse stress/repeat stability", "[mps][linearalg][native][pinverse][stress]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  std::mt19937 rng(20260324u);
  std::uniform_int_distribution<std::int64_t> dim_dist(2, 7);

  linearalg_reset_dispatch_counters();

  for (std::int64_t iter = 0; iter < 120; ++iter) {
    std::int64_t m = dim_dist(rng);
    std::int64_t n = dim_dist(rng);
    if (iter % 2 == 0) {
      m = std::max<std::int64_t>(m, n);
    } else {
      n = std::max<std::int64_t>(m, n);
    }

    const auto a_cpu = make_full_rank_matrix_(m, n, rng);
    const auto a_mps = a_cpu.to(pluss::pMPS);
    const auto ref = pinverse(a_cpu, 1e-6).to(pluss::ScalarType::Float32);
    const auto got = pinverse(a_mps, 1e-6);

    const float parity_err = max_abs_diff_(got.to(pluss::pCPU).to(pluss::ScalarType::Float32), ref);
    REQUIRE(parity_err < 1.8e-2f);

    const float penrose_resid = got.matmul(a_mps).matmul(got)
                                    .to(pluss::pCPU)
                                    .to(pluss::ScalarType::Float32)
                                    .sub(got.to(pluss::pCPU).to(pluss::ScalarType::Float32))
                                    .abs()
                                    .amax()
                                    .item<float>();
    REQUIRE(penrose_resid < 3.0e-2f);
  }

  const auto stable_cpu = pluss::make_series(
      std::vector<float>{
          2.9f, 0.1f, -0.3f, 0.5f,
          -0.2f, 3.1f, 0.4f, -0.6f,
          0.7f, -0.5f, 2.8f, 0.2f},
      {3, 4});
  const auto stable_mps = stable_cpu.to(pluss::pMPS);
  const auto ref_stable = pinverse(stable_cpu, 1e-6).to(pluss::ScalarType::Float32);

  for (std::int64_t rep = 0; rep < 220; ++rep) {
    const auto got = pinverse(stable_mps, 1e-6).to(pluss::pCPU).to(pluss::ScalarType::Float32);
    REQUIRE(max_abs_diff_(got, ref_stable) < 1.2e-2f);
  }

  const auto c = linearalg_dispatch_counters();
  REQUIRE(c.mps_bridge_total == 0);
  REQUIRE(c.mps_bridge_pinverse == 0);
}
