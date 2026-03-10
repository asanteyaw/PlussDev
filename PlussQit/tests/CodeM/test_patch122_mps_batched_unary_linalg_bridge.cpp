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
  return x.to(pluss::pCPU).to(pluss::ScalarType::Float32).contiguous().to_vec<float>();
}

void require_close_(const std::vector<float>& got, const std::vector<float>& want, float atol, const char* label) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    CAPTURE(label, i, got[i], want[i], atol);
    REQUIRE(std::fabs(got[i] - want[i]) <= atol);
  }
}

pluss::Series make_spd_batch_(std::int64_t batch, std::int64_t n) {
  std::vector<float> raw_data(static_cast<std::size_t>(batch * n * n), 0.0f);
  for (std::int64_t b = 0; b < batch; ++b) {
    for (std::int64_t i = 0; i < n; ++i) {
      for (std::int64_t j = 0; j < n; ++j) {
        const auto idx = static_cast<std::size_t>(b * n * n + i * n + j);
        raw_data[idx] = 0.15f * static_cast<float>(b + 1) +
            0.21f * static_cast<float>(i + 1) -
            0.09f * static_cast<float>(j + 1) +
            ((i == j) ? 0.35f : 0.0f);
      }
    }
  }

  const auto raw = pluss::make_series(raw_data, {batch, n, n});
  std::vector<pluss::Series> spd_parts;
  spd_parts.reserve(static_cast<std::size_t>(batch));
  for (std::int64_t b = 0; b < batch; ++b) {
    const auto m = raw.select(0, b).contiguous();
    const auto eye = pluss::Series::eye(n, n, pluss::ScalarType::Float32);
    const auto spd = m.matmul(m.transpose(0, 1)).add(eye.mul(0.75));
    spd_parts.push_back(spd.to(pluss::ScalarType::Float32).contiguous());
  }
  return stack_dim0_(spd_parts).contiguous();
}

pluss::Series make_rect_batch_(std::int64_t batch, std::int64_t m, std::int64_t n) {
  std::vector<float> data(static_cast<std::size_t>(batch * m * n), 0.0f);
  for (std::int64_t b = 0; b < batch; ++b) {
    for (std::int64_t i = 0; i < m; ++i) {
      for (std::int64_t j = 0; j < n; ++j) {
        const auto idx = static_cast<std::size_t>(b * m * n + i * n + j);
        data[idx] = 0.11f * static_cast<float>(b + 1) -
            0.07f * static_cast<float>(i + 1) +
            0.19f * static_cast<float>(j + 1);
      }
    }
  }
  return pluss::make_series(data, {batch, m, n}).to(pluss::ScalarType::Float32).contiguous();
}

} // namespace

TEST_CASE("MPS bridge supports batched unary linalg families", "[mps][linearalg][bridge][batched][unary]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const ScopedEnvVar bridge_policy("PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar native_gate("PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE", "0");

  const std::int64_t batch = 3;
  const std::int64_t n = 3;
  const auto sq_cpu = make_spd_batch_(batch, n);
  const auto sq_mps = sq_cpu.to(pluss::pMPS);

  std::vector<pluss::Series> det_ref_parts;
  std::vector<pluss::Series> logdet_ref_parts;
  std::vector<pluss::Series> slogdet_ref_parts;
  std::vector<pluss::Series> inv_ref_parts;
  std::vector<pluss::Series> chol_ref_parts;
  det_ref_parts.reserve(static_cast<std::size_t>(batch));
  logdet_ref_parts.reserve(static_cast<std::size_t>(batch));
  slogdet_ref_parts.reserve(static_cast<std::size_t>(batch));
  inv_ref_parts.reserve(static_cast<std::size_t>(batch));
  chol_ref_parts.reserve(static_cast<std::size_t>(batch));

  for (std::int64_t b = 0; b < batch; ++b) {
    const auto m = sq_cpu.select(0, b).contiguous();
    det_ref_parts.push_back(det(m));
    logdet_ref_parts.push_back(logdet(m));
    slogdet_ref_parts.push_back(slogdet(m));
    inv_ref_parts.push_back(inverse(m));
    chol_ref_parts.push_back(cholesky(m, false));
  }

  const auto det_ref = stack_dim0_(det_ref_parts);
  const auto logdet_ref = stack_dim0_(logdet_ref_parts);
  const auto slogdet_ref = stack_dim0_(slogdet_ref_parts);
  const auto inv_ref = stack_dim0_(inv_ref_parts);
  const auto chol_ref = stack_dim0_(chol_ref_parts);

  linearalg_reset_dispatch_counters();
  const auto det_mps = det(sq_mps);
  const auto logdet_mps = logdet(sq_mps);
  const auto slogdet_mps = slogdet(sq_mps);
  const auto inv_mps = inverse(sq_mps);
  const auto chol_mps = cholesky(sq_mps, false);
  const auto mpow_mps = matrix_power(sq_mps, 3);
  const auto mexp_mps = matrix_exp(sq_mps);

  REQUIRE(det_mps.platform() == pluss::pMPS);
  REQUIRE(logdet_mps.platform() == pluss::pMPS);
  REQUIRE(slogdet_mps.platform() == pluss::pMPS);
  REQUIRE(inv_mps.platform() == pluss::pMPS);
  REQUIRE(chol_mps.platform() == pluss::pMPS);
  REQUIRE(mpow_mps.platform() == pluss::pMPS);
  REQUIRE(mexp_mps.platform() == pluss::pMPS);

  REQUIRE(det_mps.dim() == 1);
  REQUIRE(det_mps.sizes()[0] == batch);
  REQUIRE(logdet_mps.dim() == 1);
  REQUIRE(logdet_mps.sizes()[0] == batch);
  REQUIRE(slogdet_mps.dim() == 2);
  REQUIRE(slogdet_mps.sizes()[0] == batch);
  REQUIRE(slogdet_mps.sizes()[1] == 2);
  REQUIRE(inv_mps.dim() == 3);
  REQUIRE(inv_mps.sizes()[0] == batch);
  REQUIRE(inv_mps.sizes()[1] == n);
  REQUIRE(inv_mps.sizes()[2] == n);
  REQUIRE(chol_mps.dim() == 3);
  REQUIRE(chol_mps.sizes()[0] == batch);
  REQUIRE(mpow_mps.dim() == 3);
  REQUIRE(mpow_mps.sizes()[0] == batch);
  REQUIRE(mexp_mps.dim() == 3);
  REQUIRE(mexp_mps.sizes()[0] == batch);

  require_close_(to_vec_f32_cpu_(det_mps), to_vec_f32_cpu_(det_ref), 6.0e-3f, "det");
  require_close_(to_vec_f32_cpu_(logdet_mps), to_vec_f32_cpu_(logdet_ref), 6.0e-3f, "logdet");
  require_close_(to_vec_f32_cpu_(slogdet_mps), to_vec_f32_cpu_(slogdet_ref), 6.0e-3f, "slogdet");
  require_close_(to_vec_f32_cpu_(inv_mps), to_vec_f32_cpu_(inv_ref), 8.0e-3f, "inverse");
  require_close_(to_vec_f32_cpu_(chol_mps), to_vec_f32_cpu_(chol_ref), 8.0e-3f, "cholesky");

  const auto rect_cpu = make_rect_batch_(2, 4, 3);
  const auto rect_mps = rect_cpu.to(pluss::pMPS);
  std::vector<pluss::Series> pinv_ref_parts;
  pinv_ref_parts.reserve(2);
  for (std::int64_t b = 0; b < 2; ++b) {
    pinv_ref_parts.push_back(pinverse(rect_cpu.select(0, b).contiguous(), 1.0e-8));
  }
  const auto pinv_ref = stack_dim0_(pinv_ref_parts);
  const auto pinv_mps = pinverse(rect_mps, 1.0e-8);
  REQUIRE(pinv_mps.platform() == pluss::pMPS);
  REQUIRE(pinv_mps.dim() == 3);
  REQUIRE(pinv_mps.sizes()[0] == 2);
  REQUIRE(pinv_mps.sizes()[1] == 3);
  REQUIRE(pinv_mps.sizes()[2] == 4);
  require_close_(to_vec_f32_cpu_(pinv_mps), to_vec_f32_cpu_(pinv_ref), 2.5e-2f, "pinverse");

  const auto c = linearalg_dispatch_counters();
  REQUIRE(c.mps_bridge_det >= static_cast<std::uint64_t>(batch));
  REQUIRE(c.mps_bridge_logdet >= static_cast<std::uint64_t>(batch));
  REQUIRE(c.mps_bridge_slogdet >= static_cast<std::uint64_t>(batch));
  REQUIRE(c.mps_bridge_inverse >= static_cast<std::uint64_t>(batch));
  REQUIRE(c.mps_bridge_cholesky >= static_cast<std::uint64_t>(batch));
  REQUIRE(c.mps_bridge_pinverse >= 2u);
}
