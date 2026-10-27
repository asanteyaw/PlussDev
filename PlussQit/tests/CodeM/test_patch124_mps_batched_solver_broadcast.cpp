#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
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

pluss::Series stack_dim0_(const std::vector<pluss::Series>& parts) {
  REQUIRE(!parts.empty());
  if (parts.size() == 1) {
    return parts.front().unsqueeze(0);
  }
  std::vector<pluss::Series> tails;
  tails.reserve(parts.size() - 1);
  for (std::size_t i = 1; i < parts.size(); ++i) {
    tails.push_back(parts[i]);
  }
  return parts.front().pile(tails, 0).contiguous();
}

std::vector<float> to_vec_f32_cpu_(const pluss::Series& x) {
  return x.to(pluss::pCPU).to(pluss::ScalarType::Float32).contiguous().host<float>().array().data;
}

void require_close_(const std::vector<float>& got, const std::vector<float>& want, float atol, const char* label) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    CAPTURE(label, i, got[i], want[i], atol);
    REQUIRE(std::fabs(got[i] - want[i]) <= atol);
  }
}

pluss::Series make_spd_b11_(std::int64_t b0, std::int64_t n) {
  std::vector<float> data(static_cast<std::size_t>(b0 * n * n), 0.0f);
  for (std::int64_t b = 0; b < b0; ++b) {
    for (std::int64_t i = 0; i < n; ++i) {
      for (std::int64_t j = 0; j < n; ++j) {
        const auto idx = static_cast<std::size_t>(b * n * n + i * n + j);
        data[idx] = 0.19f * static_cast<float>(b + 1) +
            0.12f * static_cast<float>(i + 1) -
            0.06f * static_cast<float>(j + 1) +
            ((i == j) ? 0.52f : 0.0f);
      }
    }
  }
  auto raw = pluss::make_series(data, {b0, n, n}).to(pluss::ScalarType::Float32).contiguous();
  auto eye = pluss::Series::eye(n, n, pluss::ScalarType::Float32).mul(0.7);
  std::vector<pluss::Series> parts;
  parts.reserve(static_cast<std::size_t>(b0));
  for (std::int64_t b = 0; b < b0; ++b) {
    const auto m = raw.select(0, b).contiguous();
    const auto spd = m.matmul(m.transpose(0, 1)).add(eye).to(pluss::ScalarType::Float32).contiguous();
    parts.push_back(spd);
  }
  return stack_dim0_(parts).unsqueeze(1).contiguous();
}

pluss::Series make_rhs_14_(std::int64_t b1, std::int64_t n, std::int64_t nrhs) {
  std::vector<float> data(static_cast<std::size_t>(b1 * n * nrhs), 0.0f);
  for (std::int64_t b = 0; b < b1; ++b) {
    for (std::int64_t i = 0; i < n; ++i) {
      for (std::int64_t j = 0; j < nrhs; ++j) {
        const auto idx = static_cast<std::size_t>(b * n * nrhs + i * nrhs + j);
        data[idx] = 0.14f * static_cast<float>(b + 1) -
            0.08f * static_cast<float>(i + 1) +
            0.17f * static_cast<float>(j + 1);
      }
    }
  }
  return pluss::make_series(data, {1, b1, n, nrhs}).to(pluss::ScalarType::Float32).contiguous();
}

} // namespace

TEST_CASE("MPS solver bridge supports broadcasted batch dims", "[mps][linearalg][bridge][batched][broadcast][solvers]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const ScopedEnvVar bridge_policy("PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar native_gate("PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE", "0");

  constexpr std::int64_t b0 = 2;
  constexpr std::int64_t b1 = 4;
  constexpr std::int64_t n = 3;
  constexpr std::int64_t nrhs = 2;
  constexpr std::int64_t expanded_batch = b0 * b1;

  const auto a_cpu = make_spd_b11_(b0, n);     // [2,1,3,3]
  const auto rhs_cpu = make_rhs_14_(b1, n, nrhs); // [1,4,3,2]
  const auto l_cpu = cholesky(a_cpu, false);
  const auto a_cpu_f32 = a_cpu.to(pluss::ScalarType::Float32).contiguous();
  const auto rhs_cpu_f32 = rhs_cpu.to(pluss::ScalarType::Float32).contiguous();
  const auto l_cpu_f32 = l_cpu.to(pluss::ScalarType::Float32).contiguous();

  linearalg_reset_dispatch_counters();

  const auto tri_cpu = triangular_solve(l_cpu_f32, rhs_cpu_f32, false, false);
  const auto chol_cpu = cholesky_solve(l_cpu_f32, rhs_cpu_f32, false);
  const auto lu_cpu = lu_solve(a_cpu_f32, rhs_cpu_f32);

  const auto leaf_a_cpu = l_cpu_f32.select(0, 0).select(0, 0).contiguous();
  const auto leaf_b_cpu = rhs_cpu_f32.select(0, 0).select(0, 0).contiguous();
  const auto leaf_tri_cpu = triangular_solve(leaf_a_cpu, leaf_b_cpu, false, false);
  const auto leaf_tri_mps = triangular_solve(leaf_a_cpu.to(pluss::pMPS), leaf_b_cpu.to(pluss::pMPS), false, false);
  require_close_(to_vec_f32_cpu_(leaf_tri_mps), to_vec_f32_cpu_(leaf_tri_cpu), 1.2e-2f, "leaf_triangular_solve");

  const auto tri_mps = triangular_solve(
      l_cpu_f32.to(pluss::pMPS),
      rhs_cpu_f32.to(pluss::pMPS),
      false,
      false);
  const auto chol_mps = cholesky_solve(
      l_cpu_f32.to(pluss::pMPS),
      rhs_cpu_f32.to(pluss::pMPS),
      false);
  const auto lu_mps = lu_solve(
      a_cpu_f32.to(pluss::pMPS),
      rhs_cpu_f32.to(pluss::pMPS));

  REQUIRE(tri_mps.platform() == pluss::pMPS);
  REQUIRE(chol_mps.platform() == pluss::pMPS);
  REQUIRE(lu_mps.platform() == pluss::pMPS);

  const auto tri_sz = tri_mps.sizes();
  const auto chol_sz = chol_mps.sizes();
  const auto lu_sz = lu_mps.sizes();
  REQUIRE(tri_mps.dim() == 4);
  REQUIRE(chol_mps.dim() == 4);
  REQUIRE(lu_mps.dim() == 4);
  REQUIRE(tri_sz[0] == b0);
  REQUIRE(tri_sz[1] == b1);
  REQUIRE(tri_sz[2] == n);
  REQUIRE(tri_sz[3] == nrhs);
  REQUIRE(chol_sz[0] == b0);
  REQUIRE(chol_sz[1] == b1);
  REQUIRE(chol_sz[2] == n);
  REQUIRE(chol_sz[3] == nrhs);
  REQUIRE(lu_sz[0] == b0);
  REQUIRE(lu_sz[1] == b1);
  REQUIRE(lu_sz[2] == n);
  REQUIRE(lu_sz[3] == nrhs);

  require_close_(to_vec_f32_cpu_(tri_mps), to_vec_f32_cpu_(tri_cpu), 1.2e-2f, "triangular_solve");
  require_close_(to_vec_f32_cpu_(chol_mps), to_vec_f32_cpu_(chol_cpu), 1.4e-2f, "cholesky_solve");
  require_close_(to_vec_f32_cpu_(lu_mps), to_vec_f32_cpu_(lu_cpu), 1.4e-2f, "lu_solve");

  const auto c = linearalg_dispatch_counters();
  REQUIRE(c.mps_bridge_triangular_solve >= static_cast<std::uint64_t>(expanded_batch));
  REQUIRE(c.mps_bridge_cholesky_solve >= static_cast<std::uint64_t>(expanded_batch));
  REQUIRE(c.mps_bridge_lu_solve >= static_cast<std::uint64_t>(expanded_batch));
}
