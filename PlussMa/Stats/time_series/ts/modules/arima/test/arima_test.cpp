#include <pluss/ma.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::stats::ts::Arima;
using pluss::subsp::stats::ts::ArimaFit;
using pluss::subsp::stats::ts::ArimaOptions;
using pluss::subsp::stats::ts::Sarima;
using pluss::subsp::stats::ts::SarimaOptions;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

Series simulate_ar1_truth(std::int64_t n, std::uint64_t seed) {
  const ArimaFit truth{
      .mean = 0.0,
      .intercept = 0.10,
      .sigma = 0.80,
      .ar = {0.55},
      .ma = {},
      .sar = {},
      .sma = {},
      .diff_order = 0,
      .seasonal_diff_order = 0,
      .seasonal_period = 0,
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  return truth.simulate(n, f64_cpu(), seed);
}

Series simulate_sar_truth(std::int64_t n, std::uint64_t seed) {
  const ArimaFit truth{
      .mean = 0.0,
      .intercept = 0.05,
      .sigma = 0.70,
      .ar = {0.20},
      .ma = {},
      .sar = {0.35},
      .sma = {},
      .diff_order = 0,
      .seasonal_diff_order = 0,
      .seasonal_period = 12,
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  return truth.simulate(n, f64_cpu(), seed);
}

void test_arima_fit_slice() {
  const auto cfg = f64_cpu();
  const Series y = simulate_ar1_truth(4000, 20260410ULL);

  ArimaOptions opt;
  opt.ar_order = 1;
  opt.max_iterations = 1200;
  opt.max_evaluations = 12000;

  const auto fit = Arima::fit_mle(y, opt, cfg);
  expect(fit.ar.size() == 1, "arima fit: expected one AR coefficient");
  expect(std::isfinite(fit.intercept), "arima fit: finite intercept expected");
  expect(std::isfinite(fit.sigma) && fit.sigma > 0.0, "arima fit: positive sigma expected");

  const Series inf = fit.infer(y, cfg);
  const Series llobs = fit.loglik_per_obs(y, cfg);
  const Series sc = fit.score(y, cfg);
  const Series info = fit.observed_inf(y, cfg);
  const Series f = fit.forecast(10, y, cfg);

  expect(inf.sizes()[0] == y.numel(), "arima infer shape mismatch");
  expect(llobs.sizes()[0] == y.numel(), "arima loglik_per_obs shape mismatch");
  expect(sc.sizes()[0] == 2, "arima score shape mismatch for AR(1)");
  expect(info.sizes()[0] == 2 && info.sizes()[1] == 2, "arima observed_inf shape mismatch for AR(1)");
  expect(f.sizes()[0] == 10, "arima forecast shape mismatch");

  expect(inf.isfinite().all().item<double>() == 1.0, "arima infer must be finite");
  expect(f.isfinite().all().item<double>() == 1.0, "arima forecast must be finite");
}

void test_sarima_fit_slice() {
  const auto cfg = f64_cpu();
  const Series y = simulate_sar_truth(4500, 20260411ULL);

  SarimaOptions opt;
  opt.p = 1;
  opt.P = 1;
  opt.s = 12;
  opt.max_iterations = 1400;
  opt.max_evaluations = 14000;

  const auto fit = Sarima::fit_mle(y, opt, cfg);
  expect(fit.core.ar.size() == 1, "sarima fit: expected one AR coefficient");
  expect(fit.core.sar.size() == 1, "sarima fit: expected one seasonal AR coefficient");
  expect(std::isfinite(fit.core.loglik(y)), "sarima fit: finite loglik expected");

  const Series f = fit.forecast(12, y, cfg);
  expect(f.sizes()[0] == 12, "sarima forecast shape mismatch");
  expect(f.isfinite().all().item<double>() == 1.0, "sarima forecast must be finite");
}

void test_error_paths() {
  const auto cfg = f64_cpu();
  const Series y = pluss::series::randn({256}, cfg, 20260412ULL);

  bool threw = false;
  try {
    ArimaOptions bad;
    bad.seasonal_ar_order = 1;
    bad.seasonal_period = 0;
    (void)Arima::fit_mle(y, bad, cfg);
  } catch (...) {
    threw = true;
  }
  expect(threw, "arima fit should throw when seasonal order is set but seasonal_period <= 1");

  threw = false;
  try {
    auto fit = Arima::fit_mle(y, {}, cfg);
    (void)fit.forecast(0, y, cfg);
  } catch (...) {
    threw = true;
  }
  expect(threw, "arima forecast should throw for non-positive horizon");
}

void test_arimax_slice() {
  const auto cfg = f64_cpu();
  const std::int64_t n = 2400;
  const Series x = pluss::series::randn({n, 2}, cfg, 20260440ULL);
  const ArimaFit truth{
      .mean = 0.0,
      .intercept = 0.08,
      .sigma = 0.60,
      .ar = {0.45},
      .ma = {},
      .sar = {},
      .sma = {},
      .beta = {0.70, -0.35},
      .diff_order = 0,
      .seasonal_diff_order = 0,
      .seasonal_period = 0,
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  const Series y = truth.simulate(x, cfg, 20260441ULL);

  ArimaOptions opt;
  opt.ar_order = 1;
  opt.beta0 = {0.0, 0.0};
  opt.max_iterations = 1200;
  opt.max_evaluations = 12000;
  const auto fit = Arima::fit_mle(y, x, opt, cfg);

  expect(fit.beta.size() == 2, "arimax fit: expected two regression coefficients");
  expect(std::isfinite(fit.loglik(y, x)), "arimax fit: finite loglik expected");
  expect(fit.infer(y, x, cfg).isfinite().all().item<double>() == 1.0, "arimax infer must be finite");

  const Series x_future = pluss::series::randn({12, 2}, cfg, 20260442ULL);
  const Series f = fit.forecast(12, y, x, x_future, cfg);
  expect(f.sizes()[0] == 12, "arimax forecast shape mismatch");
  expect(f.isfinite().all().item<double>() == 1.0, "arimax forecast must be finite");

  bool threw = false;
  try {
    (void)fit.loglik(y);
  } catch (...) {
    threw = true;
  }
  expect(threw, "arimax loglik(y) should throw when x is required");
}

} // namespace

int main() {
  try {
    test_arima_fit_slice();
    test_sarima_fit_slice();
    test_arimax_slice();
    test_error_paths();
    std::cout << "arima_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "arima_test failed: " << e.what() << '\n';
    return 1;
  }
}
