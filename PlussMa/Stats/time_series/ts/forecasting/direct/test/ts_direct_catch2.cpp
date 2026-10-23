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
using pluss::subsp::stats::ts::CVLossMode;
using pluss::subsp::stats::ts::DirectCVForecaster;
using pluss::subsp::stats::ts::DirectForecaster;
using pluss::subsp::stats::ts::DirectForecasterOptions;
using pluss::subsp::stats::ts::DirectLearnerKind;
using pluss::subsp::stats::ts::PartitionOptions;
using pluss::subsp::stats::ts::RegressionLoss;
using pluss::subsp::stats::ts::TsPartition;

Series make_response(const Series& x, std::uint64_t seed, double noise_scale) {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const std::int64_t n = x.sizes()[0];
  const std::int64_t p = x.sizes()[1];
  const auto xv = x.to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>();
  const auto eps = pluss::series::randn({n}, cfg, seed).to_vec<double>();
  std::vector<double> y(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t t = 1; t < n; ++t) {
    const double x0_lag = xv[static_cast<std::size_t>((t - 1) * p)];
    const double x1_lag = xv[static_cast<std::size_t>((t - 1) * p + 1)];
    y[static_cast<std::size_t>(t)] =
        0.70 * y[static_cast<std::size_t>(t - 1)] + 0.20 * x0_lag - 0.10 * x1_lag + noise_scale * eps[static_cast<std::size_t>(t)];
  }
  return pluss::make_series(y, {n}, cfg);
}

double l2_norm(const Series& x) {
  const auto vals = x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
  double sum = 0.0;
  for (const double v : vals) {
    sum += v * v;
  }
  return std::sqrt(sum);
}

} // namespace

TEST_CASE("ts direct forecaster: low-noise fit is accurate", "[ts][direct][accuracy]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const std::int64_t n = 1500;
  const Series x = pluss::series::randn({n, 2}, cfg, 20260318ULL);
  const Series y = make_response(x, 20260319ULL, 0.01);

  DirectForecasterOptions opt;
  opt.lag.horizon = {1, 2};
  opt.lag.response_lags = {1};
  opt.lag.predictor_lags = {{1}};
  opt.fit_intercept = false;

  const auto mdl = DirectForecaster::fit(x, y, opt, cfg);
  const Series mse = mdl.loss(x, y, RegressionLoss::MSE, cfg);
  const auto values = mse.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  REQUIRE(values.size() == 2);
  REQUIRE(values[0] < 0.03);
  REQUIRE(values[1] < 0.05);

  const Series prep = mdl.prepared_predictors(x, y, 2, cfg);
  REQUIRE(prep.sizes()[0] == n);
  REQUIRE(prep.sizes()[1] > 0);

  const Series fcast = mdl.forecast(x, y, cfg);
  REQUIRE(fcast.sizes()[0] == 2);
  REQUIRE(fcast.isfinite().all().item<double>() == Approx(1.0));
}

TEST_CASE("ts direct forecaster: ridge regularization shrinks coefficient norm", "[ts][direct][sensitivity]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const std::int64_t n = 1200;
  const Series x = pluss::series::randn({n, 4}, cfg, 20260320ULL);
  const Series y = make_response(
      x.index_select(1,
                     pluss::make_series(std::vector<std::int64_t>{0, 1},
                                        {2},
                                        Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU))),
      20260321ULL,
      0.05);

  DirectForecasterOptions ols;
  ols.lag.horizon = {1};
  ols.lag.response_lags = {1, 2};
  ols.lag.predictor_lags = {{1, 2}};
  ols.fit_intercept = true;
  ols.learner = DirectLearnerKind::OLS;
  const auto m_ols = DirectForecaster::fit(x, y, ols, cfg);

  DirectForecasterOptions ridge = ols;
  ridge.learner = DirectLearnerKind::Ridge;
  ridge.ridge_lambda = 1e-2;
  const auto m_ridge = DirectForecaster::fit(x, y, ridge, cfg);

  const double norm_ols = l2_norm(m_ols.step(0).beta);
  const double norm_ridge = l2_norm(m_ridge.step(0).beta);
  REQUIRE(norm_ridge <= norm_ols + 1e-10);
}

TEST_CASE("ts direct forecaster: stress and crash checks", "[ts][direct][stress][crash]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const std::int64_t n = 30000;
  const Series x = pluss::series::rand({n, 6}, cfg, 20260322ULL);
  const Series y = make_response(
      x.index_select(1,
                     pluss::make_series(std::vector<std::int64_t>{0, 1},
                                        {2},
                                        Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU))),
      20260323ULL,
      0.02);

  DirectForecasterOptions opt;
  opt.lag.horizon = {1, 4, 8};
  opt.lag.response_lags = {1, 2, 3};
  opt.lag.predictor_lags = {{1, 2, 4}};
  opt.fit_intercept = true;
  opt.learner = DirectLearnerKind::Ridge;
  opt.ridge_lambda = 1e-4;
  const auto mdl = DirectForecaster::fit(x, y, opt, cfg);

  const Series pred = mdl.predict(x, y, cfg);
  REQUIRE(pred.sizes()[0] == n);
  REQUIRE(pred.sizes()[1] == 3);
  // Only the first rows, which lack some lagged value, have no prediction (the longest history needed is 8 + 4 steps).
  const Series late = pred.index_select(0, pluss::series::arange(16, n, Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU)));
  REQUIRE(late.isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(pred.isfinite().to(ScalarType::Float64).sum().item<double>() < static_cast<double>(3 * n));

  DirectForecasterOptions bad = opt;
  bad.ridge_lambda = -0.5;
  REQUIRE_THROWS(DirectForecaster::fit(x, y, bad, cfg));
  REQUIRE_THROWS(mdl.step(-1));
}

TEST_CASE("ts direct cv forecaster: overlap handling and average loss", "[ts][direct][cv][overlap]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const std::int64_t n = 2200;
  const Series x = pluss::series::randn({n, 2}, cfg, 20260401ULL);
  const Series y = make_response(x, 20260402ULL, 0.03);

  PartitionOptions popts;
  popts.direction = pluss::subsp::stats::ts::Direction::Forward;
  popts.train_size = 700;
  popts.test_size = 420;
  popts.step_size = 210;
  popts.max_lag = 8;
  const auto part = TsPartition::sliding_window(n, 4, popts);

  DirectForecasterOptions opt;
  opt.lag.horizon = {1, 2, 5};
  opt.lag.response_lags = {1, 2, 3};
  opt.lag.predictor_lags = {{1, 2}};
  opt.fit_intercept = true;
  opt.learner = DirectLearnerKind::Ridge;
  opt.ridge_lambda = 1e-4;

  const auto cv = DirectCVForecaster::fit(x, y, part, opt, cfg);
  const Series pred = cv.cvpredict(x, y, cfg);
  const Series lavg = cv.cvloss(x, y, CVLossMode::Average, RegressionLoss::MSE, cfg);

  REQUIRE(pred.sizes()[0] == n);
  REQUIRE(pred.sizes()[1] == 3);
  REQUIRE(lavg.sizes()[0] == 3);

  const auto pvals = pred.to(pluss::pCPU, ScalarType::Float64).reshape({pred.numel()}).to_vec<double>();
  const auto finite_count =
      static_cast<std::int64_t>(std::count_if(pvals.begin(), pvals.end(), [](double v) { return std::isfinite(v); }));
  REQUIRE(finite_count > 0);
  REQUIRE(finite_count < pred.numel());

  const auto lvals = lavg.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  REQUIRE(std::all_of(lvals.begin(), lvals.end(), [](double v) { return std::isfinite(v) && v >= 0.0; }));
}

TEST_CASE("ts direct cv forecaster: stress and crash checks", "[ts][direct][cv][stress][crash]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const std::int64_t n = 30000;
  const Series x = pluss::series::rand({n, 4}, cfg, 20260403ULL);
  const Series y = make_response(
      x.index_select(1,
                     pluss::make_series(std::vector<std::int64_t>{0, 1},
                                        {2},
                                        Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU))),
      20260404ULL,
      0.02);

  PartitionOptions popts;
  popts.direction = pluss::subsp::stats::ts::Direction::Forward;
  popts.train_size = 12000;
  popts.test_size = 9000;
  popts.step_size = 4000;
  popts.max_lag = 8;
  const auto part = TsPartition::sliding_window(n, 3, popts);

  DirectForecasterOptions opt;
  opt.lag.horizon = {1, 3, 7};
  opt.lag.response_lags = {1, 2, 4};
  opt.lag.predictor_lags = {{1, 2}};
  opt.fit_intercept = true;
  opt.learner = DirectLearnerKind::Ridge;
  opt.ridge_lambda = 1e-4;

  const auto cv = DirectCVForecaster::fit(x, y, part, opt, cfg);
  const Series lind = cv.cvloss(x, y, CVLossMode::Individual, RegressionLoss::MAE, cfg);
  REQUIRE(lind.sizes()[0] == 3);
  REQUIRE(lind.sizes()[1] == 3);
  REQUIRE(lind.isfinite().all().item<double>() == Approx(1.0));

  REQUIRE_THROWS(cv.cvpredict(x, pluss::series::rand({n + 1}, cfg, 101ULL), cfg));
  REQUIRE_THROWS(cv.fold_model(-1));
}

// Reference values: ordinary least squares on the rows that have a complete set of lagged predictors, computed independently.
TEST_CASE("direct forecaster fits only complete rows and blanks the rest", "[ts][direct][reference]") {
  const int n = 30;
  std::vector<double> x(2 * n), y(n);
  for (int i = 0; i < n; ++i) {
    const double x1 = std::sin(0.7 * i) + 0.1 * i, x2 = std::cos(0.4 * i * i * 0.1) + i % 3;
    x[static_cast<std::size_t>(2 * i)] = x1;
    x[static_cast<std::size_t>(2 * i + 1)] = x2;
    y[static_cast<std::size_t>(i)] = 0.5 * x1 + std::sin(1.3 * i) + 0.05 * i * i * 0.1;
  }
  const Series xs = pluss::make_series(x, {n, 2}).to(ScalarType::Float64);
  const Series ys = pluss::make_series(y, {n}).to(ScalarType::Float64);

  DirectForecasterOptions o;
  o.lag.horizon = {1, 3};
  o.lag.response_lags = {1, 2};
  o.lag.predictor_lags = {{1, 2}, {1}};
  const auto f = DirectForecaster::fit(xs, ys, o);

  const std::vector<std::vector<double>> beta{{0.257642, 0.434179, 0.108200, -0.183314, 1.077306, -0.427419},
                                              {1.180341, 0.369032, -0.640171, -0.314757, -0.230175, 1.365510}};
  for (int s = 0; s < 2; ++s) {
    const auto b = f.step(s).beta.to_vec<double>();
    for (std::size_t j = 0; j < beta[static_cast<std::size_t>(s)].size(); ++j) REQUIRE(b[j] == Approx(beta[static_cast<std::size_t>(s)][j]).epsilon(1e-5));
  }

  // The first rows lack some lag: their prepared predictors and predictions are NaN, not zero-filled.
  const auto prepared = f.prepared_predictors(xs, ys, 1).to_vec<double>();
  REQUIRE(std::isnan(prepared[1]));
  const auto pred = f.predict(xs, ys).to_vec<double>();
  REQUIRE(std::isnan(pred[0]));
  REQUIRE(std::isfinite(pred[2 * (n - 1)]));
  // The loss averages over the rows that have a prediction.
  REQUIRE(std::isfinite(f.loss(xs, ys).to_vec<double>()[0]));
}

// A forecast is made from the last observation: with the coefficients above, the one-step forecast of Y(n + 1) uses the last observed values as the first lags.
TEST_CASE("direct forecaster forecasts from the last observation", "[ts][direct][forecast][reference]") {
  const int n = 30;
  std::vector<double> x(2 * n), y(n);
  for (int i = 0; i < n; ++i) {
    const double x1 = std::sin(0.7 * i) + 0.1 * i, x2 = std::cos(0.4 * i * i * 0.1) + i % 3;
    x[static_cast<std::size_t>(2 * i)] = x1;
    x[static_cast<std::size_t>(2 * i + 1)] = x2;
    y[static_cast<std::size_t>(i)] = 0.5 * x1 + std::sin(1.3 * i) + 0.05 * i * i * 0.1;
  }
  const Series xs = pluss::make_series(x, {n, 2}).to(ScalarType::Float64);
  const Series ys = pluss::make_series(y, {n}).to(ScalarType::Float64);

  DirectForecasterOptions o;
  o.lag.horizon = {1, 3};
  o.lag.response_lags = {1, 2};
  o.lag.predictor_lags = {{1, 2}, {1}};
  const auto f = DirectForecaster::fit(xs, ys, o).forecast(xs, ys).to_vec<double>();
  REQUIRE(f.size() == 2);
  REQUIRE(f[0] == Approx(6.689346).epsilon(1e-5));
  REQUIRE(f[1] == Approx(4.948242).epsilon(1e-5));

  // Leading predictor with lags 0 and 1: its values at the forecast times are supplied as x_future.
  DirectForecasterOptions lead;
  lead.lag.horizon = {1, 2};
  lead.lag.response_lags = {1, 3};
  lead.lag.leading_predictors = {0};
  lead.lag.predictor_lags = {{1}};
  lead.lag.leading_predictor_lags = {{0, 1}};
  const auto model = DirectForecaster::fit(xs, ys, lead);
  REQUIRE_THROWS(model.forecast(xs, ys));
  const Series future = pluss::make_series(std::vector<double>{1.7, 0.4, 2.2, 0.9}, {2, 2}).to(ScalarType::Float64);
  const auto g = model.forecast(xs, ys, future).to_vec<double>();
  REQUIRE(g[0] == Approx(4.958580).epsilon(1e-5));
  REQUIRE(g[1] == Approx(4.801808).epsilon(1e-5));
}
