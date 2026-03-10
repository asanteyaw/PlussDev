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

void require_close_vec_(const std::vector<float>& got, const std::vector<float>& want, float atol) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(std::fabs(got[i] - want[i]) <= atol);
  }
}

std::vector<float> matmul_row_major_(
    const std::vector<float>& a, std::int64_t m, std::int64_t k,
    const std::vector<float>& b, std::int64_t n) {
  std::vector<float> out(static_cast<std::size_t>(m * n), 0.0f);
  for (std::int64_t i = 0; i < m; ++i) {
    for (std::int64_t p = 0; p < k; ++p) {
      const float av = a[static_cast<std::size_t>(i * k + p)];
      const std::size_t bbase = static_cast<std::size_t>(p * n);
      const std::size_t obase = static_cast<std::size_t>(i * n);
      for (std::int64_t j = 0; j < n; ++j) {
        out[obase + static_cast<std::size_t>(j)] += av * b[bbase + static_cast<std::size_t>(j)];
      }
    }
  }
  return out;
}

std::vector<float> qr_reconstruct_(const pluss::Series& q, const pluss::Series& r) {
  const auto q_cpu = q.to(pluss::pCPU).to(pluss::ScalarType::Float32).contiguous();
  const auto r_cpu = r.to(pluss::pCPU).to(pluss::ScalarType::Float32).contiguous();
  const auto q_sizes = q_cpu.sizes();
  const auto r_sizes = r_cpu.sizes();
  const auto qv = q_cpu.to_vec<float>();
  const auto rv = r_cpu.to_vec<float>();
  return matmul_row_major_(qv, q_sizes[0], q_sizes[1], rv, r_sizes[1]);
}

std::vector<float> svd_reconstruct_reduced_(const pluss::Series& u, const pluss::Series& s, const pluss::Series& vh) {
  const auto u_cpu = u.to(pluss::pCPU).to(pluss::ScalarType::Float32).contiguous();
  const auto s_cpu = s.to(pluss::pCPU).to(pluss::ScalarType::Float32).contiguous();
  const auto vh_cpu = vh.to(pluss::pCPU).to(pluss::ScalarType::Float32).contiguous();

  const auto us = u_cpu.sizes();
  const auto vhs = vh_cpu.sizes();
  const std::int64_t m = us[0];
  const std::int64_t k = us[1];
  const std::int64_t n = vhs[1];

  auto uv = u_cpu.to_vec<float>();
  const auto sv = s_cpu.to_vec<float>();
  const auto vhv = vh_cpu.to_vec<float>();

  for (std::int64_t i = 0; i < m; ++i) {
    for (std::int64_t j = 0; j < k; ++j) {
      uv[static_cast<std::size_t>(i * k + j)] *= sv[static_cast<std::size_t>(j)];
    }
  }
  return matmul_row_major_(uv, m, k, vhv, n);
}

void run_case_(
    const pluss::Series& a_cpu,
    float qr_atol,
    float svd_atol,
    std::int64_t& expected_qr_bridge,
    std::int64_t& expected_svd_bridge) {
  using namespace pluss::subsp::linearalg;

  const auto a_mps = a_cpu.to(pluss::pMPS);
  const auto shape = a_cpu.sizes();
  const std::int64_t batch = shape[0];
  const std::int64_t m = shape[1];
  const std::int64_t n = shape[2];
  const std::int64_t k = std::min(m, n);

  const auto q = qr_q(a_mps, true);
  const auto r = qr_r(a_mps, true);
  REQUIRE(q.platform() == pluss::pMPS);
  REQUIRE(r.platform() == pluss::pMPS);
  REQUIRE(q.dim() == 3);
  REQUIRE(r.dim() == 3);
  REQUIRE(q.sizes()[0] == batch);
  REQUIRE(q.sizes()[1] == m);
  REQUIRE(q.sizes()[2] == k);
  REQUIRE(r.sizes()[0] == batch);
  REQUIRE(r.sizes()[1] == k);
  REQUIRE(r.sizes()[2] == n);
  const auto q_cpu = q.to(pluss::pCPU).to(pluss::ScalarType::Float32).contiguous();
  const auto r_cpu = r.to(pluss::pCPU).to(pluss::ScalarType::Float32).contiguous();

  for (std::int64_t b = 0; b < batch; ++b) {
    CAPTURE(b, m, n);
    const auto recon = qr_reconstruct_(q_cpu.select(0, b), r_cpu.select(0, b));
    const auto ref = a_cpu.select(0, b).to(pluss::ScalarType::Float32).to_vec<float>();
    require_close_vec_(recon, ref, qr_atol);
  }

  const auto u = svd_u(a_mps, false);
  const auto s = svd_s(a_mps, false);
  const auto vh = svd_vh(a_mps, false);
  REQUIRE(u.platform() == pluss::pMPS);
  REQUIRE(s.platform() == pluss::pMPS);
  REQUIRE(vh.platform() == pluss::pMPS);
  REQUIRE(u.dim() == 3);
  REQUIRE(s.dim() == 2);
  REQUIRE(vh.dim() == 3);
  REQUIRE(u.sizes()[0] == batch);
  REQUIRE(u.sizes()[1] == m);
  REQUIRE(u.sizes()[2] == k);
  REQUIRE(s.sizes()[0] == batch);
  REQUIRE(s.sizes()[1] == k);
  REQUIRE(vh.sizes()[0] == batch);
  REQUIRE(vh.sizes()[1] == k);
  REQUIRE(vh.sizes()[2] == n);
  const auto u_cpu = u.to(pluss::pCPU).to(pluss::ScalarType::Float32).contiguous();
  const auto s_cpu = s.to(pluss::pCPU).to(pluss::ScalarType::Float32).contiguous();
  const auto vh_cpu = vh.to(pluss::pCPU).to(pluss::ScalarType::Float32).contiguous();

  for (std::int64_t b = 0; b < batch; ++b) {
    CAPTURE(b, m, n);
    const auto recon = svd_reconstruct_reduced_(u_cpu.select(0, b), s_cpu.select(0, b), vh_cpu.select(0, b));
    const auto ref = a_cpu.select(0, b).to(pluss::ScalarType::Float32).to_vec<float>();
    require_close_vec_(recon, ref, svd_atol);
  }

  expected_qr_bridge += 2 * batch;
  expected_svd_bridge += 3 * batch;
}

} // namespace

TEST_CASE("MPS linalg qr/svd bridge supports batched matrices", "[mps][linearalg][bridge][batched][qr][svd]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const ScopedEnvVar bridge_policy("PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar native_gate("PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE", "0");

  linearalg_reset_dispatch_counters();
  std::int64_t expected_qr_bridge = 0;
  std::int64_t expected_svd_bridge = 0;

  // Wide matrices (m < n)
  const auto wide_cpu = pluss::make_series(
      std::vector<float>{
          0.2f, -0.1f, 0.3f, 0.7f, -0.2f,
          0.4f, 0.5f, -0.6f, 0.1f, 0.2f,
          -0.3f, 0.2f, 0.8f, -0.5f, 0.6f,
          0.9f, -0.7f, 0.1f, 0.4f, -0.8f,

          -0.5f, 0.2f, 0.6f, -0.4f, 0.3f,
          0.7f, -0.9f, 0.8f, 0.2f, -0.1f,
          0.1f, 0.3f, -0.2f, 0.5f, -0.6f,
          -0.4f, 0.6f, -0.7f, 0.9f, 0.2f},
      {2, 4, 5});
  run_case_(wide_cpu, 1.0e-2f, 5.0e-2f, expected_qr_bridge, expected_svd_bridge);

  // Tall matrices (m > n)
  const auto tall_cpu = pluss::make_series(
      std::vector<float>{
          0.4f, -0.3f, 0.2f,
          -0.7f, 0.6f, -0.1f,
          0.9f, 0.5f, -0.8f,
          0.1f, -0.2f, 0.3f,
          -0.4f, 0.7f, -0.6f,
          0.8f, -0.5f, 0.4f,

          -0.2f, 0.1f, 0.9f,
          0.3f, -0.4f, 0.7f,
          -0.8f, 0.6f, -0.5f,
          0.5f, 0.2f, -0.3f,
          0.6f, -0.7f, 0.4f,
          -0.9f, 0.8f, -0.1f},
      {2, 6, 3});
  run_case_(tall_cpu, 1.0e-2f, 5.0e-2f, expected_qr_bridge, expected_svd_bridge);

  const auto c = linearalg_dispatch_counters();
  REQUIRE(c.mps_bridge_qr >= static_cast<std::uint64_t>(expected_qr_bridge));
  REQUIRE(c.mps_bridge_svd >= static_cast<std::uint64_t>(expected_svd_bridge));
  REQUIRE(c.mps_bridge_total >= static_cast<std::uint64_t>(expected_qr_bridge + expected_svd_bridge));
}
