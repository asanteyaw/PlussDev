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
using pluss::subsp::stat::ts::CVLossMode;
using pluss::subsp::stat::ts::DirectCVForecaster;
using pluss::subsp::stat::ts::DirectForecaster;
using pluss::subsp::stat::ts::DirectForecasterOptions;
using pluss::subsp::stat::ts::DirectLearnerKind;
using pluss::subsp::stat::ts::PartitionOptions;
using pluss::subsp::stat::ts::RegressionLoss;
using pluss::subsp::stat::ts::TsPartition;

Series make_response(const Series& x, std::uint64_t seed, double noise_scale) {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const std::int64_t n = x.sizes()[0];
  const std::int64_t p = x.sizes()[1];
  const auto xv = x.to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>();
  const auto eps = pluss::randn({n}, cfg, seed).to_vec<double>();
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
  const Series x = pluss::randn({n, 2}, cfg, 20260318ULL);
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
  const Series x = pluss::randn({n, 4}, cfg, 20260320ULL);
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
  const Series x = pluss::rand({n, 6}, cfg, 20260322ULL);
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
  REQUIRE(pred.isfinite().all().item<double>() == Approx(1.0));

  DirectForecasterOptions bad = opt;
  bad.ridge_lambda = -0.5;
  REQUIRE_THROWS(DirectForecaster::fit(x, y, bad, cfg));
  REQUIRE_THROWS(mdl.step(-1));
}

TEST_CASE("ts direct cv forecaster: overlap handling and average loss", "[ts][direct][cv][overlap]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const std::int64_t n = 2200;
  const Series x = pluss::randn({n, 2}, cfg, 20260401ULL);
  const Series y = make_response(x, 20260402ULL, 0.03);

  PartitionOptions popts;
  popts.direction = pluss::subsp::stat::ts::Direction::Forward;
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
  const Series x = pluss::rand({n, 4}, cfg, 20260403ULL);
  const Series y = make_response(
      x.index_select(1,
                     pluss::make_series(std::vector<std::int64_t>{0, 1},
                                        {2},
                                        Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU))),
      20260404ULL,
      0.02);

  PartitionOptions popts;
  popts.direction = pluss::subsp::stat::ts::Direction::Forward;
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

  REQUIRE_THROWS(cv.cvpredict(x, pluss::rand({n + 1}, cfg, 101ULL), cfg));
  REQUIRE_THROWS(cv.fold_model(-1));
}
