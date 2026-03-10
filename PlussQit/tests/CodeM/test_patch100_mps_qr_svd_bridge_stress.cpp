#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

namespace {

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

} // namespace

TEST_CASE("MPS linalg qr/svd bridge deterministic parity and counters", "[mps][linearalg][bridge][qr][svd]") {
  using namespace pluss::subsp::linearalg;

  linearalg_reset_dispatch_counters();

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const auto a_cpu = pluss::make_series(
      std::vector<float>{
          1.0f, 2.0f, -1.0f,
          2.0f, 0.0f, 3.0f,
          -1.0f, 1.5f, 0.5f,
          0.5f, -2.0f, 1.0f},
      {4, 3});
  const auto a_mps = a_cpu.to(pluss::pMPS);
  const auto a_ref = a_cpu.to(pluss::ScalarType::Float32).to_vec<float>();

  const auto q_mps = qr_q(a_mps, true);
  const auto r_mps = qr_r(a_mps, true);
  REQUIRE(q_mps.platform() == pluss::pMPS);
  REQUIRE(r_mps.platform() == pluss::pMPS);
  require_close_vec_(qr_reconstruct_(q_mps, r_mps), a_ref, 5e-3f);

  const auto u_cpu = svd_u(a_cpu, false).to(pluss::ScalarType::Float32);
  const auto s_cpu = svd_s(a_cpu, false).to(pluss::ScalarType::Float32);
  const auto vh_cpu = svd_vh(a_cpu, false).to(pluss::ScalarType::Float32);
  const auto u_mps = svd_u(a_mps, false);
  const auto s_mps = svd_s(a_mps, false);
  const auto vh_mps = svd_vh(a_mps, false);
  REQUIRE(u_mps.platform() == pluss::pMPS);
  REQUIRE(s_mps.platform() == pluss::pMPS);
  REQUIRE(vh_mps.platform() == pluss::pMPS);
  require_close_vec_(s_mps.to(pluss::pCPU).to(pluss::ScalarType::Float32).to_vec<float>(), s_cpu.to_vec<float>(), 6e-2f);
  require_close_vec_(svd_reconstruct_reduced_(u_mps, s_mps, vh_mps), a_ref, 4e-2f);
  require_close_vec_(svd_reconstruct_reduced_(u_cpu, s_cpu, vh_cpu), a_ref, 7e-3f);

  const auto c = linearalg_dispatch_counters();
  REQUIRE(c.mps_bridge_qr == 0);
  REQUIRE(c.mps_bridge_svd == 0);
  REQUIRE(c.mps_bridge_total == 0);
}

TEST_CASE("MPS linalg qr/svd bridge stress stability", "[mps][linearalg][bridge][qr][svd][stress]") {
  using namespace pluss::subsp::linearalg;

  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  std::mt19937 rng(20260322u);
  std::uniform_int_distribution<std::int64_t> m_dist(2, 6);
  std::uniform_int_distribution<std::int64_t> n_dist(2, 6);
  std::uniform_real_distribution<float> val_dist(-1.0f, 1.0f);

  for (std::int64_t iter = 0; iter < 80; ++iter) {
    const auto m = m_dist(rng);
    const auto n = n_dist(rng);
    std::vector<float> data(static_cast<std::size_t>(m * n), 0.0f);
    for (auto& v : data) v = val_dist(rng);

    const auto a_cpu = pluss::make_series(data, {m, n});
    const auto a_mps = a_cpu.to(pluss::pMPS);
    const auto a_ref = a_cpu.to(pluss::ScalarType::Float32).to_vec<float>();

    const auto q_mps = qr_q(a_mps, true);
    const auto r_mps = qr_r(a_mps, true);
    require_close_vec_(qr_reconstruct_(q_mps, r_mps), a_ref, 8e-3f);

    const auto u_mps = svd_u(a_mps, false);
    const auto s_mps = svd_s(a_mps, false);
    const auto vh_mps = svd_vh(a_mps, false);
    require_close_vec_(svd_reconstruct_reduced_(u_mps, s_mps, vh_mps), a_ref, 4e-2f);
  }
}
