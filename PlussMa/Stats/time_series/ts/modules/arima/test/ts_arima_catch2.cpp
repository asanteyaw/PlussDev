#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/ma.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace {

using Catch::Approx;
using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::stats::ts::Arima;
using pluss::subsp::stats::ts::ArimaFit;
using pluss::subsp::stats::ts::ArimaOptions;
using pluss::subsp::stats::ts::Sarima;
using pluss::subsp::stats::ts::SarimaOptions;

Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

Series simulate_arima(std::int64_t n, std::uint64_t seed) {
  const ArimaFit truth{
      .mean = 0.0,
      .intercept = 0.10,
      .sigma = 0.85,
      .ar = {0.55},
      .ma = {-0.20},
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

Series simulate_sarima(std::int64_t n, std::uint64_t seed) {
  const ArimaFit truth{
      .mean = 0.0,
      .intercept = 0.02,
      .sigma = 0.70,
      .ar = {0.20},
      .ma = {},
      .sar = {0.35},
      .sma = {0.10},
      .diff_order = 0,
      .seasonal_diff_order = 0,
      .seasonal_period = 12,
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  return truth.simulate(n, f64_cpu(), seed);
}

} // namespace

TEST_CASE("ts arima: parameter recovery and score behavior", "[ts][arima][accuracy]") {
  const auto cfg = f64_cpu();
  const Series y = simulate_arima(5500, 20260413ULL);

  ArimaOptions opt;
  opt.ar_order = 1;
  opt.ma_order = 1;
  opt.max_iterations = 1200;
  opt.max_evaluations = 12000;

  const auto fit = Arima::fit_mle(y, opt, cfg);
  const Series sc = fit.score(y, cfg);
  const double snorm = sc.square().sum().sqrt().item<double>();

  REQUIRE(fit.ar.size() == 1);
  REQUIRE(fit.ma.size() == 1);
  REQUIRE(std::isfinite(fit.intercept));
  REQUIRE(std::isfinite(fit.sigma));
  REQUIRE(fit.sigma > 0.0);
  REQUIRE(std::abs(fit.ar[0]) < 1.0);
  REQUIRE(std::abs(fit.ma[0]) < 1.0);
  REQUIRE(std::isfinite(snorm));
  REQUIRE(snorm < 2000.0);
}

TEST_CASE("ts arima: seasonal numerics and finite outputs", "[ts][sarima][numerics]") {
  const auto cfg = f64_cpu();
  const Series y = simulate_sarima(4200, 20260414ULL);

  SarimaOptions opt;
  opt.p = 1;
  opt.P = 1;
  opt.Q = 1;
  opt.s = 12;
  opt.max_iterations = 1400;
  opt.max_evaluations = 14000;

  const auto fit = Sarima::fit_mle(y, opt, cfg);
  const Series inf = fit.infer(y, cfg);
  const Series llobs = fit.loglik_per_obs(y, cfg);
  const Series info = fit.observed_inf(y, cfg);
  const Series f = fit.forecast(18, y, cfg);

  REQUIRE(fit.core.ar.size() == 1);
  REQUIRE(fit.core.sar.size() == 1);
  REQUIRE(fit.core.sma.size() == 1);
  REQUIRE(inf.isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(f.isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(info.isfinite().all().item<double>() == Approx(1.0));

  const auto lv = llobs.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  const auto finite_count = static_cast<std::int64_t>(std::count_if(lv.begin(), lv.end(), [](double v) { return std::isfinite(v); }));
  REQUIRE(finite_count > static_cast<std::int64_t>(lv.size() / 2));
}

TEST_CASE("ts arima: differencing edge path and crash checks", "[ts][arima][edge][crash]") {
  const auto cfg = f64_cpu();
  const ArimaFit integrated_truth{
      .mean = 0.0,
      .intercept = 0.01,
      .sigma = 0.40,
      .ar = {},
      .ma = {},
      .sar = {},
      .sma = {},
      .diff_order = 1,
      .seasonal_diff_order = 0,
      .seasonal_period = 0,
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  const Series y = integrated_truth.simulate(1200, cfg, 20260415ULL);

  ArimaOptions opt;
  opt.diff_order = 1;
  opt.max_iterations = 900;
  opt.max_evaluations = 9000;
  const auto fit = Arima::fit_mle(y, opt, cfg);

  const auto lv = fit.loglik_per_obs(y, cfg).to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  REQUIRE(lv.size() == static_cast<std::size_t>(y.numel()));
  const auto it = std::find_if(lv.begin(), lv.end(), [](double v) { return std::isfinite(v); });
  REQUIRE(it != lv.end());
  REQUIRE(std::all_of(it, lv.end(), [](double v) { return std::isfinite(v); }));

  REQUIRE_THROWS(fit.forecast(0, y, cfg));
  ArimaOptions bad;
  bad.seasonal_ar_order = 1;
  bad.seasonal_period = 1;
  REQUIRE_THROWS(Arima::fit_mle(y, bad, cfg));
}

TEST_CASE("ts arimax: exogenous regression path", "[ts][arimax][accuracy]") {
  const auto cfg = f64_cpu();
  const std::int64_t n = 3000;
  const Series x = pluss::series::randn({n, 2}, cfg, 20260443ULL);
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
  const Series y = truth.simulate(x, cfg, 20260444ULL);

  ArimaOptions opt;
  opt.ar_order = 1;
  opt.beta0 = {0.0, 0.0};
  opt.max_iterations = 1200;
  opt.max_evaluations = 12000;
  const auto fit = Arima::fit_mle(y, x, opt, cfg);

  REQUIRE(fit.beta.size() == 2);
  REQUIRE(std::isfinite(fit.loglik(y, x)));
  const auto lv = fit.loglik_per_obs(y, x, cfg).to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  const auto finite_count = static_cast<std::int64_t>(std::count_if(lv.begin(), lv.end(), [](double v) { return std::isfinite(v); }));
  REQUIRE(finite_count > static_cast<std::int64_t>(lv.size() / 2));
  REQUIRE(fit.infer(y, x, cfg).isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(fit.score(y, x, cfg).isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(fit.observed_inf(y, x, cfg).isfinite().all().item<double>() == Approx(1.0));

  const Series x_future = pluss::series::randn({16, 2}, cfg, 20260445ULL);
  const Series f = fit.forecast(16, y, x, x_future, cfg);
  REQUIRE(f.sizes()[0] == 16);
  REQUIRE(f.isfinite().all().item<double>() == Approx(1.0));

  REQUIRE_THROWS(fit.loglik(y));
  REQUIRE_THROWS(fit.forecast(16, y, cfg));
}

// Reference values from an independent implementation of the exact Gaussian likelihood (Kalman filter, stationary start) on a
// deterministic series: e_t = frac(sin(12.9898 t + 78.233) * 43758.5453) - 0.5.
namespace {
std::vector<double> reference_noise(int n) {
  std::vector<double> e(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    const double t = i + 1.0;
    double u = std::sin(12.9898 * t + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[static_cast<std::size_t>(i)] = u - 0.5;
  }
  return e;
}
} // namespace

TEST_CASE("arima maximises the exact likelihood and matches reference estimates", "[ts][arima][exact]") {
  namespace ts = pluss::subsp::stats::ts;
  const auto e = reference_noise(200);
  std::vector<double> x(200);
  double carry = 0.0;
  for (int i = 0; i < 200; ++i) {
    carry = 0.7 * carry + e[static_cast<std::size_t>(i)];
    x[static_cast<std::size_t>(i)] = carry;
  }
  const auto y = pluss::make_series(x, {200}).to(pluss::ScalarType::Float64);

  // started from the default (all coefficients zero), the fit must still find the autoregressive coefficient
  ts::ArimaOptions options;
  options.ar_order = 1;
  const auto ar1 = ts::Arima::fit_mle(y, options);
  REQUIRE(ar1.ar[0] == Catch::Approx(0.650630).margin(5e-4));
  REQUIRE(ar1.sigma * ar1.sigma == Catch::Approx(0.080685).margin(2e-4));
  REQUIRE(ar1.loglik(y) == Catch::Approx(-32.346807).margin(2e-3));

  options.ar_order = 1;
  options.ma_order = 1;
  const auto arma = ts::Arima::fit_mle(y, options);
  REQUIRE(arma.loglik(y) == Catch::Approx(-32.039876).margin(2e-3));

  // the conditional likelihood is a different (larger) number: it leaves out the first observation
  options.ma_order = 0;
  options.exact_likelihood = false;
  const auto conditional = ts::Arima::fit_mle(y, options);
  REQUIRE(conditional.loglik(y) > ar1.loglik(y));
}

TEST_CASE("sarima multiplies the seasonal and non-seasonal factors", "[ts][sarima][exact]") {
  namespace ts = pluss::subsp::stats::ts;
  const auto e = reference_noise(240);
  std::vector<double> y(240, 0.0);
  for (int i = 0; i < 240; ++i) {
    y[static_cast<std::size_t>(i)] = e[static_cast<std::size_t>(i)] + (i >= 1 ? 0.5 * y[static_cast<std::size_t>(i - 1)] : 0.0) +
                                     (i >= 12 ? 0.6 * y[static_cast<std::size_t>(i - 12)] : 0.0) -
                                     (i >= 13 ? 0.3 * y[static_cast<std::size_t>(i - 13)] : 0.0);
  }
  const auto series = pluss::make_series(y, {240}).to(pluss::ScalarType::Float64);
  ts::SarimaOptions options;
  options.p = 1;
  options.P = 1;
  options.s = 12;
  const auto fit = ts::Sarima::fit_mle(series, options);
  REQUIRE(fit.core.ar[0] == Catch::Approx(0.449195).margin(2e-3));
  REQUIRE(fit.core.sar[0] == Catch::Approx(0.562212).margin(2e-3));
  REQUIRE(fit.loglik(series) == Catch::Approx(-40.128722).margin(5e-3));
}
