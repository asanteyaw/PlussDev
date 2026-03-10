#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace {

using Catch::Approx;
using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::stat::ts::Arima;
using pluss::subsp::stat::ts::ArimaFit;
using pluss::subsp::stat::ts::ArimaOptions;
using pluss::subsp::stat::ts::Sarima;
using pluss::subsp::stat::ts::SarimaOptions;

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
