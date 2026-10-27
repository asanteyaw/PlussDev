#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <algorithm>
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

void require_close_(const std::vector<float>& got, const std::vector<float>& want, float atol) {
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
  const auto qv = q_cpu.host<float>().array().data;
  const auto rv = r_cpu.host<float>().array().data;
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

  auto uv = u_cpu.host<float>().array().data;
  const auto sv = s_cpu.host<float>().array().data;
  const auto vhv = vh_cpu.host<float>().array().data;

  for (std::int64_t i = 0; i < m; ++i) {
    for (std::int64_t j = 0; j < k; ++j) {
      uv[static_cast<std::size_t>(i * k + j)] *= sv[static_cast<std::size_t>(j)];
    }
  }
  return matmul_row_major_(uv, m, k, vhv, n);
}

} // namespace

TEST_CASE("MPS argsort survives strict fallback policy roundtrip", "[mps][sort][argsort][policy][strict]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const ScopedEnvVar strict_policy("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");

  std::mt19937 rng(20260310u);
  std::uniform_int_distribution<std::int64_t> rows_dist(1, 10);
  std::uniform_int_distribution<std::int64_t> cols_dist(1, 12);
  std::uniform_real_distribution<float> val_dist(-10.0f, 10.0f);

  for (std::int64_t iter = 0; iter < 48; ++iter) {
    const auto rows = rows_dist(rng);
    const auto cols = cols_dist(rng);
    std::vector<float> data(static_cast<std::size_t>(rows * cols), 0.0f);
    for (auto& v : data) v = val_dist(rng);

    const auto x_cpu = pluss::make_series(data, {rows, cols});
    const auto x_mps = x_cpu.to(pluss::pMPS);
    const std::int64_t dim = (iter % 2 == 0) ? 0 : 1;
    const bool descending = (iter % 3 == 0);

    const auto argsort_mps = x_mps.argsort(dim, descending, /*stable=*/false);
    REQUIRE(argsort_mps.platform() == pluss::pMPS);
    const auto got = argsort_mps.to(pluss::pCPU).host<std::int64_t>().array().data;
    const auto want = x_cpu.argsort(dim, descending, /*stable=*/false).host<std::int64_t>().array().data;
    REQUIRE(got == want);
  }
}

TEST_CASE("MPS bridge split decomposition stays coherent across repeated calls", "[mps][linearalg][bridge][cache][coherency]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const ScopedEnvVar bridge_policy("PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE", "1");
  const ScopedEnvVar native_gate("PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE", "0");

  std::mt19937 rng(20260331u);
  std::uniform_int_distribution<std::int64_t> m_dist(2, 7);
  std::uniform_int_distribution<std::int64_t> n_dist(2, 7);
  std::uniform_real_distribution<float> val_dist(-1.0f, 1.0f);

  for (std::int64_t iter = 0; iter < 36; ++iter) {
    const auto m = m_dist(rng);
    const auto n = n_dist(rng);
    CAPTURE(iter, m, n);
    std::vector<float> data(static_cast<std::size_t>(m * n), 0.0f);
    for (auto& v : data) v = val_dist(rng);

    const auto a_cpu = pluss::make_series(data, {m, n});
    const auto a_mps = a_cpu.to(pluss::pMPS);
    const auto a_ref = a_cpu.to(pluss::ScalarType::Float32).host<float>().array().data;

    const auto q1 = qr_q(a_mps, true);
    const auto r1 = qr_r(a_mps, true);
    const auto q2 = qr_q(a_mps, true);
    const auto r2 = qr_r(a_mps, true);
    require_close_(qr_reconstruct_(q1, r1), a_ref, 1.0e-2f);
    require_close_(qr_reconstruct_(q2, r2), a_ref, 1.0e-2f);

    const auto u1 = svd_u(a_mps, false);
    const auto s1 = svd_s(a_mps, false);
    const auto vh1 = svd_vh(a_mps, false);
    const auto u2 = svd_u(a_mps, false);
    const auto s2 = svd_s(a_mps, false);
    const auto vh2 = svd_vh(a_mps, false);

    const auto recon1 = svd_reconstruct_reduced_(u1, s1, vh1);
    const auto recon2 = svd_reconstruct_reduced_(u2, s2, vh2);
    require_close_(recon1, a_ref, 5.0e-2f);
    require_close_(recon2, a_ref, 5.0e-2f);
    require_close_(recon1, recon2, 5.0e-4f);
  }
}
