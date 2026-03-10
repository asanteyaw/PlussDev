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
using pluss::subsp::stats::ts::AcfNorm;
using pluss::subsp::stats::ts::AcfOptions;
using pluss::subsp::stats::ts::LjungBoxOptions;
using pluss::subsp::stats::ts::PacfOptions;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

Series make_ar1(std::int64_t n, double phi, std::uint64_t seed) {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const auto eps = pluss::randn({n}, cfg, seed).to_vec<double>();
  std::vector<double> y(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t t = 1; t < n; ++t) {
    y[static_cast<std::size_t>(t)] = phi * y[static_cast<std::size_t>(t - 1)] + eps[static_cast<std::size_t>(t)];
  }
  return pluss::make_series(y, {n}, cfg);
}

void test_acf_white_noise() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series y = pluss::randn({4000}, cfg, 20260309ULL);

  AcfOptions opt;
  opt.max_lag = 20;
  opt.norm = AcfNorm::Biased;
  const auto out = pluss::subsp::stats::ts::acf(y, opt, cfg).to_vec<double>();

  expect(out.size() == 21, "acf size mismatch");
  expect(std::abs(out[0] - 1.0) < 1e-12, "acf lag-0 must be 1");
  for (std::size_t k = 1; k < 10; ++k) {
    expect(std::abs(out[k]) < 0.08, "acf white-noise lag should stay small");
  }
}

void test_pacf_ar1() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series y = make_ar1(6000, 0.8, 20260310ULL);

  PacfOptions opt;
  opt.max_lag = 10;
  opt.norm = AcfNorm::Biased;
  const auto out = pluss::subsp::stats::ts::pacf(y, opt, cfg).to_vec<double>();

  expect(out.size() == 11, "pacf size mismatch");
  expect(std::abs(out[0] - 1.0) < 1e-12, "pacf lag-0 must be 1");
  expect(std::abs(out[1] - 0.8) < 0.06, "pacf lag-1 should recover AR(1) coefficient");
  expect(std::abs(out[2]) < 0.08, "pacf lag-2 should be near zero for AR(1)");
}

void test_ljung_box_signal_vs_noise() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series wn = pluss::randn({3500}, cfg, 20260311ULL);
  const Series ar = make_ar1(3500, 0.7, 20260312ULL);

  LjungBoxOptions opt;
  opt.lags = {10, 20};
  const auto lb_wn = pluss::subsp::stats::ts::ljung_box(wn, opt, cfg);
  const auto lb_ar = pluss::subsp::stats::ts::ljung_box(ar, opt, cfg);
  const auto p_wn = lb_wn.pvalue.to_vec<double>();
  const auto p_ar = lb_ar.pvalue.to_vec<double>();

  expect(p_wn.size() == 2, "ljung_box output size mismatch");
  expect(p_ar.size() == 2, "ljung_box output size mismatch");
  expect(p_wn[1] > 1e-3, "white-noise Ljung-Box p-value should not be extremely tiny");
  expect(p_ar[1] < 1e-10, "AR(1) Ljung-Box p-value should be tiny");
}

void test_error_paths() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series y = pluss::randn({32}, cfg, 20260313ULL);

  bool threw = false;
  try {
    AcfOptions bad;
    bad.max_lag = -1;
    (void)pluss::subsp::stats::ts::acf(y, bad, cfg);
  } catch (...) {
    threw = true;
  }
  expect(threw, "acf should throw when max_lag < 0");

  threw = false;
  try {
    LjungBoxOptions bad;
    bad.lags = {0, 5};
    (void)pluss::subsp::stats::ts::ljung_box(y, bad, cfg);
  } catch (...) {
    threw = true;
  }
  expect(threw, "ljung_box should throw for non-positive lag");
}

} // namespace

int main() {
  try {
    test_acf_white_noise();
    test_pacf_ar1();
    test_ljung_box_signal_vs_noise();
    test_error_paths();
    std::cout << "diagnostics_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "diagnostics_test failed: " << e.what() << '\n';
    return 1;
  }
}
