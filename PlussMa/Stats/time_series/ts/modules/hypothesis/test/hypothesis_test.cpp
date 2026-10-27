#include <pluss/ma.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::stats::ts::AdfModel;
using pluss::subsp::stats::ts::AdfOptions;
using pluss::subsp::stats::ts::ArchLmOptions;
using pluss::subsp::stats::ts::DmLoss;
using pluss::subsp::stats::ts::DmOptions;
using pluss::subsp::stats::ts::DmTail;
using pluss::subsp::stats::ts::KpssOptions;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

Series make_ar1(std::int64_t n, double phi, std::uint64_t seed) {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const auto eps = pluss::series::randn({n}, cfg, seed).host<double>();
  std::vector<double> y(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t t = 1; t < n; ++t) {
    y[static_cast<std::size_t>(t)] = phi * y[static_cast<std::size_t>(t - 1)] + eps[static_cast<std::size_t>(t)];
  }
  return pluss::make_series(y, {n}, cfg);
}

Series make_random_walk(std::int64_t n, std::uint64_t seed) {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const auto eps = pluss::series::randn({n}, cfg, seed).host<double>();
  std::vector<double> y(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t t = 1; t < n; ++t) {
    y[static_cast<std::size_t>(t)] = y[static_cast<std::size_t>(t - 1)] + eps[static_cast<std::size_t>(t)];
  }
  return pluss::make_series(y, {n}, cfg);
}

Series make_arch1(std::int64_t n, double omega, double alpha, std::uint64_t seed) {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const auto z = pluss::series::randn({n}, cfg, seed).host<double>();
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

void test_arch_lm_detects_arch_effects() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series wn = pluss::series::randn({5000}, cfg, 20260321ULL);
  const Series arch = make_arch1(5000, 0.1, 0.7, 20260322ULL);

  ArchLmOptions opt;
  opt.lags = 8;
  const auto null_res = pluss::subsp::stats::ts::arch_lm(wn, opt, cfg);
  const auto alt_res = pluss::subsp::stats::ts::arch_lm(arch, opt, cfg);

  expect(null_res.pvalue > 1e-3, "arch_lm white-noise p-value should not be tiny");
  expect(alt_res.pvalue < 1e-10, "arch_lm ARCH(1) p-value should be tiny");
}

void test_adf_stationary_vs_unit_root() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series ar = make_ar1(5000, 0.75, 20260323ULL);
  const Series rw = make_random_walk(5000, 20260324ULL);

  AdfOptions opt;
  opt.lags = 1;
  opt.model = AdfModel::ARD;
  opt.alpha = 0.05;

  const auto adf_ar = pluss::subsp::stats::ts::adf_test(ar, opt, cfg);
  const auto adf_rw = pluss::subsp::stats::ts::adf_test(rw, opt, cfg);

  expect(adf_ar.reject, "adf_test should reject unit root for stationary AR(1)");
  expect(!adf_rw.reject, "adf_test should fail to reject unit root for random walk");
}

void test_kpss_stationary_vs_unit_root() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series ar = make_ar1(5000, 0.7, 20260325ULL);
  const Series rw = make_random_walk(5000, 20260326ULL);

  KpssOptions opt;
  opt.lags = 8;
  opt.trend = false;
  opt.alpha = 0.05;

  const auto kpss_ar = pluss::subsp::stats::ts::kpss_test(ar, opt, cfg);
  const auto kpss_rw = pluss::subsp::stats::ts::kpss_test(rw, opt, cfg);

  expect(!kpss_ar.reject, "kpss_test should fail to reject stationarity for AR(1)");
  expect(kpss_rw.reject, "kpss_test should reject stationarity for random walk");
}

void test_dm_forecast_loss_difference() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series base = pluss::series::randn({4000}, cfg, 20260406ULL);
  const Series err_b = base;
  const Series err_a = base.add(1.0);

  DmOptions opt;
  opt.loss = DmLoss::Squared;
  opt.horizon = 1;
  opt.tail = DmTail::Right;
  opt.alpha = 0.01;

  const auto right = pluss::subsp::stats::ts::dm_test(err_a, err_b, opt, cfg);
  expect(right.reject, "dm_test should reject when model A has larger squared-error loss");
  expect(right.pvalue < 1e-8, "dm_test right-tail p-value should be tiny");
  expect(right.stat > 0.0, "dm_test statistic should be positive for larger model A loss");

  opt.tail = DmTail::Left;
  const auto left = pluss::subsp::stats::ts::dm_test(err_b, err_a, opt, cfg);
  expect(left.reject, "dm_test left-tail should reject in swapped ordering");
  expect(left.pvalue < 1e-8, "dm_test left-tail p-value should be tiny");
  expect(left.stat < 0.0, "dm_test swapped statistic should be negative");
}

void test_error_paths() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series y = pluss::series::randn({64}, cfg, 20260420ULL);

  bool threw = false;
  try {
    ArchLmOptions bad;
    bad.lags = 0;
    (void)pluss::subsp::stats::ts::arch_lm(y, bad, cfg);
  } catch (...) {
    threw = true;
  }
  expect(threw, "arch_lm should throw when lags <= 0");

  threw = false;
  try {
    AdfOptions bad;
    bad.lags = -1;
    (void)pluss::subsp::stats::ts::adf_test(y, bad, cfg);
  } catch (...) {
    threw = true;
  }
  expect(threw, "adf_test should throw when lags < 0");

  threw = false;
  try {
    KpssOptions bad;
    bad.lags = -2;
    (void)pluss::subsp::stats::ts::kpss_test(y, bad, cfg);
  } catch (...) {
    threw = true;
  }
  expect(threw, "kpss_test should throw when lags < 0");

  threw = false;
  try {
    DmOptions bad;
    bad.horizon = 0;
    (void)pluss::subsp::stats::ts::dm_test(y, y, bad, cfg);
  } catch (...) {
    threw = true;
  }
  expect(threw, "dm_test should throw when horizon < 1");
}

} // namespace

int main() {
  try {
    test_arch_lm_detects_arch_effects();
    test_adf_stationary_vs_unit_root();
    test_kpss_stationary_vs_unit_root();
    test_dm_forecast_loss_difference();
    test_error_paths();
    std::cout << "hypothesis_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "hypothesis_test failed: " << e.what() << '\n';
    return 1;
  }
}
