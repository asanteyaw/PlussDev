#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

namespace {

void require_close_vec_(const std::vector<float>& got, const std::vector<float>& want, float atol = 1e-4f) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(std::fabs(got[i] - want[i]) <= atol);
  }
}

} // namespace

TEST_CASE("MPS take parity and stress", "[mps][take][stress]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  std::mt19937 rng(20260311u);
  std::uniform_real_distribution<float> val_dist(-100.0f, 100.0f);
  std::uniform_int_distribution<std::int64_t> n_dist(1, 256);

  for (std::int64_t iter = 0; iter < 220; ++iter) {
    const auto n = n_dist(rng);
    std::vector<float> data(static_cast<std::size_t>(n));
    for (auto& v : data) v = val_dist(rng);

    std::uniform_int_distribution<std::int64_t> idx_dist(-n, n - 1);
    const auto k = 1 + (iter % 64);
    std::vector<std::int64_t> idx_data(static_cast<std::size_t>(k));
    for (auto& i : idx_data) i = idx_dist(rng);

    const auto x_cpu = pluss::make_series(data, {n});
    const auto idx_cpu = pluss::make_series(idx_data, {k});
    const auto ref = x_cpu.take(idx_cpu);

    const auto x_mps = x_cpu.to(pluss::pMPS);
    const auto idx_mps = idx_cpu.to(pluss::pMPS);
    const auto got = x_mps.take(idx_mps).to(pluss::pCPU);

    require_close_vec_(got.to_vec<float>(), ref.to_vec<float>());
  }
}

TEST_CASE("MPS put parity and stress", "[mps][put][stress]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  std::mt19937 rng(20260312u);
  std::uniform_real_distribution<float> val_dist(-40.0f, 40.0f);
  std::uniform_int_distribution<std::int64_t> n_dist(1, 192);

  for (std::int64_t iter = 0; iter < 220; ++iter) {
    const auto n = n_dist(rng);
    std::vector<float> base_data(static_cast<std::size_t>(n));
    for (auto& v : base_data) v = val_dist(rng);

    const auto k = 1 + (iter % 64);
    std::uniform_int_distribution<std::int64_t> idx_dist(-n, n - 1);
    std::vector<std::int64_t> idx_data(static_cast<std::size_t>(k));
    for (auto& i : idx_data) i = idx_dist(rng);

    const bool scalar_src = (iter % 3 == 0);
    std::vector<float> src_data(scalar_src ? 1u : static_cast<std::size_t>(k));
    for (auto& v : src_data) v = val_dist(rng);

    const auto base_cpu = pluss::make_series(base_data, {n});
    const auto idx_cpu = pluss::make_series(idx_data, {k});
    const auto src_cpu = pluss::make_series(src_data, {static_cast<std::int64_t>(src_data.size())});

    auto ref_plain = base_cpu.clone();
    ref_plain.put_(idx_cpu, src_cpu, /*accumulate=*/false);
    auto ref_accum = base_cpu.clone();
    ref_accum.put_(idx_cpu, src_cpu, /*accumulate=*/true);

    const auto idx_mps = idx_cpu.to(pluss::pMPS);
    const auto src_mps = src_cpu.to(pluss::pMPS);
    auto out_plain = base_cpu.to(pluss::pMPS);
    out_plain.put_(idx_mps, src_mps, /*accumulate=*/false);
    auto out_accum = base_cpu.to(pluss::pMPS);
    out_accum.put_(idx_mps, src_mps, /*accumulate=*/true);

    require_close_vec_(out_plain.to(pluss::pCPU).to_vec<float>(), ref_plain.to_vec<float>());
    require_close_vec_(out_accum.to(pluss::pCPU).to_vec<float>(), ref_accum.to_vec<float>());
  }
}

TEST_CASE("MPS put duplicate-index semantics match CPU for accumulate modes", "[mps][put][duplicates][stress]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }

  const auto base_cpu = pluss::make_series(
      std::vector<float>{0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f},
      {8});
  const auto idx_cpu = pluss::make_series(
      std::vector<std::int64_t>{1, -7, 1, -7, 3, 3, -5},
      {7});
  const auto src_cpu = pluss::make_series(
      std::vector<float>{10.0f, 20.0f, -3.0f, 5.0f, 9.0f, -2.0f, 4.0f},
      {7});

  auto ref_plain = base_cpu.clone();
  ref_plain.put_(idx_cpu, src_cpu, /*accumulate=*/false);
  auto ref_accum = base_cpu.clone();
  ref_accum.put_(idx_cpu, src_cpu, /*accumulate=*/true);

  const auto idx_mps = idx_cpu.to(pluss::pMPS);
  const auto src_mps = src_cpu.to(pluss::pMPS);

  for (std::int64_t iter = 0; iter < 120; ++iter) {
    auto out_plain = base_cpu.to(pluss::pMPS);
    out_plain.put_(idx_mps, src_mps, /*accumulate=*/false);

    auto out_accum = base_cpu.to(pluss::pMPS);
    out_accum.put_(idx_mps, src_mps, /*accumulate=*/true);

    require_close_vec_(out_plain.to(pluss::pCPU).to_vec<float>(), ref_plain.to_vec<float>());
    require_close_vec_(out_accum.to(pluss::pCPU).to_vec<float>(), ref_accum.to_vec<float>());
  }
}
