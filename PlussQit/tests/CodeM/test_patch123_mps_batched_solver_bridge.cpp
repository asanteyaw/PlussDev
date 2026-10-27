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

void require_close_(const std::vector<float>& got, const std::vector<float>& want, float atol) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(std::fabs(got[i] - want[i]) <= atol);
  }
}

std::vector<float> to_vec_f32_cpu_(const pluss::Series& x) {
  return x.to(pluss::pCPU).to(pluss::ScalarType::Float32).contiguous().host<float>().array().data;
}

pluss::Series make_spd_batch_(std::int64_t batch, std::int64_t n) {
  std::vector<float> raw_data(static_cast<std::size_t>(batch * n * n), 0.0f);
  for (std::int64_t b = 0; b < batch; ++b) {
    for (std::int64_t i = 0; i < n; ++i) {
      for (std::int64_t j = 0; j < n; ++j) {
        const auto idx = static_cast<std::size_t>(b * n * n + i * n + j);
        raw_data[idx] = 0.17f * static_cast<float>(b + 1) +
            0.13f * static_cast<float>(i + 1) -
            0.05f * static_cast<float>(j + 1) +
            ((i == j) ? 0.45f : 0.0f);
      }
    }
  }

  const auto raw = pluss::make_series(raw_data, {batch, n, n});
  std::vector<pluss::Series> spd_parts;
  spd_parts.reserve(static_cast<std::size_t>(batch));
  for (std::int64_t b = 0; b < batch; ++b) {
    const auto m = raw.select(0, b).contiguous();
    const auto eye = pluss::Series::eye(n, n, pluss::ScalarType::Float32);
    spd_parts.push_back(m.matmul(m.transpose(0, 1)).add(eye.mul(0.8)).to(pluss::ScalarType::Float32).contiguous());
  }
  return stack_dim0_(spd_parts).contiguous();
}

} // namespace

TEST_CASE("MPS bridge solver family supports batched matrices", "[mps][linearalg][bridge][batched][solvers]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const ScopedEnvVar bridge_policy("PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar native_gate("PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE", "0");

  constexpr std::int64_t batch = 2;
  constexpr std::int64_t n = 3;
  constexpr std::int64_t nrhs = 2;

  const auto a_cpu = make_spd_batch_(batch, n);
  std::vector<pluss::Series> l_parts;
  l_parts.reserve(batch);
  for (std::int64_t b = 0; b < batch; ++b) {
    l_parts.push_back(cholesky(a_cpu.select(0, b).contiguous(), false).to(pluss::ScalarType::Float32).contiguous());
  }
  const auto l_cpu = stack_dim0_(l_parts);

  const auto rhs_cpu = pluss::make_series(
      std::vector<float>{
          0.3f, -0.2f, 0.4f, 0.1f, -0.5f, 0.6f,
          -0.1f, 0.5f, 0.7f, -0.3f, 0.2f, 0.8f},
      {batch, n, nrhs});

  std::vector<pluss::Series> tri_ref_parts;
  std::vector<pluss::Series> chol_ref_parts;
  std::vector<pluss::Series> chol_inv_ref_parts;
  std::vector<pluss::Series> lu_ref_parts;
  tri_ref_parts.reserve(batch);
  chol_ref_parts.reserve(batch);
  chol_inv_ref_parts.reserve(batch);
  lu_ref_parts.reserve(batch);
  for (std::int64_t b = 0; b < batch; ++b) {
    const auto li = l_cpu.select(0, b).contiguous();
    const auto ai = a_cpu.select(0, b).contiguous();
    const auto ri = rhs_cpu.select(0, b).contiguous();
    tri_ref_parts.push_back(triangular_solve(li, ri, /*upper=*/false, /*unitriangular=*/false));
    chol_ref_parts.push_back(cholesky_solve(li, ri, /*upper=*/false));
    chol_inv_ref_parts.push_back(cholesky_inverse(li, /*upper=*/false));
    lu_ref_parts.push_back(lu_solve(ai, ri));
  }
  const auto tri_ref = stack_dim0_(tri_ref_parts);
  const auto chol_ref = stack_dim0_(chol_ref_parts);
  const auto chol_inv_ref = stack_dim0_(chol_inv_ref_parts);
  const auto lu_ref = stack_dim0_(lu_ref_parts);

  linearalg_reset_dispatch_counters();
  const auto l_mps = l_cpu.to(pluss::pMPS);
  const auto a_mps = a_cpu.to(pluss::pMPS);
  const auto rhs_mps = rhs_cpu.to(pluss::pMPS);

  const auto tri_mps = triangular_solve(l_mps, rhs_mps, /*upper=*/false, /*unitriangular=*/false);
  const auto chol_mps = cholesky_solve(l_mps, rhs_mps, /*upper=*/false);
  const auto chol_inv_mps = cholesky_inverse(l_mps, /*upper=*/false);
  const auto lu_mps = lu_solve(a_mps, rhs_mps);

  REQUIRE(tri_mps.platform() == pluss::pMPS);
  REQUIRE(chol_mps.platform() == pluss::pMPS);
  REQUIRE(chol_inv_mps.platform() == pluss::pMPS);
  REQUIRE(lu_mps.platform() == pluss::pMPS);

  REQUIRE(tri_mps.sizes()[0] == batch);
  REQUIRE(chol_mps.sizes()[0] == batch);
  REQUIRE(chol_inv_mps.sizes()[0] == batch);
  REQUIRE(lu_mps.sizes()[0] == batch);

  require_close_(to_vec_f32_cpu_(tri_mps), to_vec_f32_cpu_(tri_ref), 1.0e-2f);
  require_close_(to_vec_f32_cpu_(chol_mps), to_vec_f32_cpu_(chol_ref), 1.2e-2f);
  require_close_(to_vec_f32_cpu_(chol_inv_mps), to_vec_f32_cpu_(chol_inv_ref), 1.2e-2f);
  require_close_(to_vec_f32_cpu_(lu_mps), to_vec_f32_cpu_(lu_ref), 1.2e-2f);

  const auto c = linearalg_dispatch_counters();
  REQUIRE(c.mps_bridge_triangular_solve >= static_cast<std::uint64_t>(batch));
  REQUIRE(c.mps_bridge_cholesky_solve >= static_cast<std::uint64_t>(batch));
  REQUIRE(c.mps_bridge_cholesky_inverse >= static_cast<std::uint64_t>(batch));
  REQUIRE(c.mps_bridge_lu_solve >= static_cast<std::uint64_t>(batch));
}
