#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

namespace {

class ScopedEnvVar final {
 public:
  ScopedEnvVar(const char* key, const char* value) : key_(key) {
    if (const char* existing = std::getenv(key_); existing != nullptr) {
      had_previous_ = true;
      previous_ = existing;
    }
    if (value != nullptr) {
      ::setenv(key_, value, 1);
    } else {
      ::unsetenv(key_);
    }
  }

  ~ScopedEnvVar() {
    if (had_previous_) {
      ::setenv(key_, previous_.c_str(), 1);
    } else {
      ::unsetenv(key_);
    }
  }

 private:
  const char* key_{nullptr};
  bool had_previous_{false};
  std::string previous_{};
};

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
    data[static_cast<std::size_t>(i * n + i)] += 2.4f;
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

TEST_CASE("MPS native reduced SVD strict route parity and counters", "[mps][linearalg][native][svd][strict]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const ScopedEnvVar native_gate("PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE", "1");
  const ScopedEnvVar strict_bridge("PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE", "0");
  const ScopedEnvVar native_route("PLUSS_MPS_LINALG_SOLVER_ROUTE", "native");
  const ScopedEnvVar enable_svd_device("PLUSS_MPS_LINALG_NATIVE_DEVICE_SVD", "1");

  linearalg_reset_dispatch_counters();

  const auto a_cpu = pluss::make_series(
      std::vector<float>{
          2.8f, -0.2f, 0.5f,
          0.1f, 3.0f, -0.4f,
          -0.3f, 0.6f, 2.5f,
          0.7f, -0.1f, 0.9f},
      {4, 3});
  const auto a_mps = a_cpu.to(pluss::pMPS);

  const auto u = svd_u(a_mps, false);
  const auto s = svd_s(a_mps, false);
  const auto vh = svd_vh(a_mps, false);

  REQUIRE(u.platform() == pluss::pMPS);
  REQUIRE(s.platform() == pluss::pMPS);
  REQUIRE(vh.platform() == pluss::pMPS);

  const auto recon = svd_reconstruct_(u, s, vh).to(pluss::ScalarType::Float32);
  REQUIRE(max_abs_diff_(recon, a_cpu.to(pluss::ScalarType::Float32)) < 6e-2f);

  const auto c = linearalg_dispatch_counters();
  REQUIRE(c.mps_bridge_total == 0);
  REQUIRE(c.mps_bridge_svd == 0);
  REQUIRE(c.mps_native_svd_device_success >= 1);
  REQUIRE(c.mps_native_svd_device_fallback == 0);
}

TEST_CASE("MPS native reduced SVD strict route stress/repeat is crash-free", "[mps][linearalg][native][svd][strict][stress]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const ScopedEnvVar native_gate("PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE", "1");
  const ScopedEnvVar strict_bridge("PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE", "0");
  const ScopedEnvVar native_route("PLUSS_MPS_LINALG_SOLVER_ROUTE", "native");
  const ScopedEnvVar enable_svd_device("PLUSS_MPS_LINALG_NATIVE_DEVICE_SVD", "1");

  std::mt19937 rng(20260327u);
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
    REQUIRE(max_abs_diff_(recon, a_cpu.to(pluss::ScalarType::Float32)) < 8e-2f);

    const auto s_vec = s.to(pluss::pCPU).to(pluss::ScalarType::Float32).to_vec<float>();
    for (const auto sv : s_vec) {
      REQUIRE(sv >= -1e-6f);
    }
  }

  const auto stable_cpu = pluss::make_series(
      std::vector<float>{
          3.1f, -0.2f, 0.4f, 0.1f,
          0.3f, 2.7f, -0.5f, 0.2f,
          -0.4f, 0.7f, 3.0f, -0.3f},
      {3, 4});
  const auto stable_mps = stable_cpu.to(pluss::pMPS);
  const auto ref = stable_cpu.to(pluss::ScalarType::Float32);

  for (std::int64_t rep = 0; rep < 200; ++rep) {
    const auto u = svd_u(stable_mps, false);
    const auto s = svd_s(stable_mps, false);
    const auto vh = svd_vh(stable_mps, false);
    const auto recon = svd_reconstruct_(u, s, vh).to(pluss::ScalarType::Float32);
    REQUIRE(max_abs_diff_(recon, ref) < 8e-2f);
  }

  const auto c = linearalg_dispatch_counters();
  REQUIRE(c.mps_bridge_total == 0);
  REQUIRE(c.mps_bridge_svd == 0);
  REQUIRE(c.mps_native_svd_device_fallback == 0);
}

TEST_CASE("MPS native reduced SVD strict route rejects host fallback", "[mps][linearalg][native][svd][strict][policy]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const ScopedEnvVar native_gate("PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE", "1");
  const ScopedEnvVar strict_bridge("PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE", "0");
  const ScopedEnvVar native_route("PLUSS_MPS_LINALG_SOLVER_ROUTE", "native");
  const ScopedEnvVar disable_svd_device("PLUSS_MPS_LINALG_NATIVE_DEVICE_SVD", "0");

  linearalg_reset_dispatch_counters();

  const auto a_cpu = pluss::make_series(
      std::vector<float>{
          2.4f, -0.1f, 0.3f,
          0.2f, 2.9f, -0.4f,
          -0.2f, 0.5f, 2.6f,
          0.6f, -0.2f, 0.8f},
      {4, 3});
  const auto a_mps = a_cpu.to(pluss::pMPS);

  REQUIRE_THROWS(svd_u(a_mps, false));

  const auto c = linearalg_dispatch_counters();
  REQUIRE(c.mps_bridge_total == 0);
  REQUIRE(c.mps_native_svd_device_success == 0);
  REQUIRE(c.mps_native_svd_device_fallback >= 1);
}
