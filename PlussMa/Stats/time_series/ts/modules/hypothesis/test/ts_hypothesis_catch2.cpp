#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/ma.h>

#include <cmath>
#include <cstdint>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::stats::ts::AdfModel;
using pluss::subsp::stats::ts::AdfOptions;
using pluss::subsp::stats::ts::ArchLmOptions;
using pluss::subsp::stats::ts::DmLoss;
using pluss::subsp::stats::ts::DmOptions;
using pluss::subsp::stats::ts::DmTail;
using pluss::subsp::stats::ts::KpssOptions;

Series make_ar1(std::int64_t n, double phi, std::uint64_t seed) {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const auto eps = pluss::randn({n}, cfg, seed).to_vec<double>();
  std::vector<double> y(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t t = 1; t < n; ++t) {
    y[static_cast<std::size_t>(t)] = phi * y[static_cast<std::size_t>(t - 1)] + eps[static_cast<std::size_t>(t)];
  }
  return pluss::make_series(y, {n}, cfg);
}

Series make_random_walk(std::int64_t n, std::uint64_t seed) {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const auto eps = pluss::randn({n}, cfg, seed).to_vec<double>();
  std::vector<double> y(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t t = 1; t < n; ++t) {
    y[static_cast<std::size_t>(t)] = y[static_cast<std::size_t>(t - 1)] + eps[static_cast<std::size_t>(t)];
  }
  return pluss::make_series(y, {n}, cfg);
}

Series make_arch1(std::int64_t n, double omega, double alpha, std::uint64_t seed) {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const auto z = pluss::randn({n}, cfg, seed).to_vec<double>();
  std::vector<double> y(static_cast<std::size_t>(n), 0.0);
  std::vector<double> v(static_cast<std::size_t>(n), omega / std::max(1e-9, 1.0 - alpha));
  for (std::int64_t t = 1; t < n; ++t) {
    v[static_cast<std::size_t>(t)] =
        omega + alpha * y[static_cast<std::size_t>(t - 1)] * y[static_cast<std::size_t>(t - 1)];
    y[static_cast<std::size_t>(t)] = std::sqrt(std::max(1e-12, v[static_cast<std::size_t>(t)])) *
                                     z[static_cast<std::size_t>(t)];
  }
  return pluss::make_series(y, {n}, cfg);
}

} // namespace

TEST_CASE("ts hypothesis: ARCH-LM detects conditional heteroscedasticity", "[ts][hypothesis][archlm]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series wn = pluss::randn({5000}, cfg, 20260331ULL);
  const Series arch = make_arch1(5000, 0.1, 0.7, 20260401ULL);

  ArchLmOptions opt;
  opt.lags = 8;
  const auto null_res = pluss::subsp::stats::ts::arch_lm(wn, opt, cfg);
  const auto alt_res = pluss::subsp::stats::ts::arch_lm(arch, opt, cfg);

  REQUIRE(null_res.pvalue > 1e-3);
  REQUIRE(alt_res.pvalue < 1e-10);
}

TEST_CASE("ts hypothesis: ADF separates stationary and unit-root paths", "[ts][hypothesis][adf]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series ar = make_ar1(5000, 0.75, 20260402ULL);
  const Series rw = make_random_walk(5000, 20260403ULL);

  AdfOptions opt;
  opt.lags = 1;
  opt.model = AdfModel::ARD;
  opt.alpha = 0.05;

  const auto adf_ar = pluss::subsp::stats::ts::adf_test(ar, opt, cfg);
  const auto adf_rw = pluss::subsp::stats::ts::adf_test(rw, opt, cfg);

  REQUIRE(adf_ar.reject);
  REQUIRE_FALSE(adf_rw.reject);
}

TEST_CASE("ts hypothesis: KPSS separates stationary and unit-root paths", "[ts][hypothesis][kpss]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series ar = make_ar1(5000, 0.7, 20260404ULL);
  const Series rw = make_random_walk(5000, 20260405ULL);

  KpssOptions opt;
  opt.lags = 8;
  opt.trend = false;
  opt.alpha = 0.05;

  const auto kpss_ar = pluss::subsp::stats::ts::kpss_test(ar, opt, cfg);
  const auto kpss_rw = pluss::subsp::stats::ts::kpss_test(rw, opt, cfg);

  REQUIRE_FALSE(kpss_ar.reject);
  REQUIRE(kpss_rw.reject);
}

TEST_CASE("ts hypothesis: DM test detects forecast loss difference", "[ts][hypothesis][dm]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series base = pluss::randn({4000}, cfg, 20260406ULL);
  const Series err_b = base;
  const Series err_a = base.add(1.0);

  DmOptions opt;
  opt.loss = DmLoss::Squared;
  opt.horizon = 1;
  opt.tail = DmTail::Right;
  opt.alpha = 0.01;

  const auto right = pluss::subsp::stats::ts::dm_test(err_a, err_b, opt, cfg);
  REQUIRE(right.reject);
  REQUIRE(right.pvalue < 1e-8);
  REQUIRE(right.stat > 0.0);

  opt.tail = DmTail::Left;
  const auto left = pluss::subsp::stats::ts::dm_test(err_b, err_a, opt, cfg);
  REQUIRE(left.reject);
  REQUIRE(left.pvalue < 1e-8);
  REQUIRE(left.stat < 0.0);
}

TEST_CASE("ts hypothesis: parameter validation", "[ts][hypothesis][errors]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series y = pluss::randn({64}, cfg, 20260421ULL);

  ArchLmOptions bad_arch;
  bad_arch.lags = 0;
  REQUIRE_THROWS(pluss::subsp::stats::ts::arch_lm(y, bad_arch, cfg));

  AdfOptions bad_adf;
  bad_adf.lags = -1;
  REQUIRE_THROWS(pluss::subsp::stats::ts::adf_test(y, bad_adf, cfg));

  KpssOptions bad_kpss;
  bad_kpss.lags = -1;
  REQUIRE_THROWS(pluss::subsp::stats::ts::kpss_test(y, bad_kpss, cfg));

  DmOptions bad_dm;
  bad_dm.horizon = 0;
  REQUIRE_THROWS(pluss::subsp::stats::ts::dm_test(y, y, bad_dm, cfg));
}
