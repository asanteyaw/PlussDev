#include <pluss/pluss.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::stat::ts::CVLossMode;
using pluss::subsp::stat::ts::DirectCVForecaster;
using pluss::subsp::stat::ts::DirectForecasterOptions;
using pluss::subsp::stat::ts::PartitionOptions;
using pluss::subsp::stat::ts::RegressionLoss;
using pluss::subsp::stat::ts::TsPartition;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

Series make_arx_response(const Series& x, std::uint64_t seed, double noise_scale) {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const auto xv = x.to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>();
  const auto eps = pluss::randn({x.sizes()[0]}, cfg, seed).to_vec<double>();
  const std::int64_t n = x.sizes()[0];
  const std::int64_t p = x.sizes()[1];
  std::vector<double> y(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t t = 1; t < n; ++t) {
    const double x0_lag = xv[static_cast<std::size_t>((t - 1) * p)];
    const double x1_lag = xv[static_cast<std::size_t>((t - 1) * p + 1)];
    y[static_cast<std::size_t>(t)] =
        0.60 * y[static_cast<std::size_t>(t - 1)] + 0.25 * x0_lag - 0.12 * x1_lag + noise_scale * eps[static_cast<std::size_t>(t)];
  }
  return pluss::make_series(y, {n}, cfg);
}

void test_cvpredict_and_losses() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const std::int64_t n = 1500;
  const Series x = pluss::randn({n, 2}, cfg, 20260326ULL);
  const Series y = make_arx_response(x, 20260327ULL, 0.03);

  PartitionOptions popts;
  popts.direction = pluss::subsp::stat::ts::Direction::Forward;
  popts.train_size = 500;
  popts.test_size = 300;
  popts.step_size = 200;
  popts.max_lag = 8;
  const auto part = TsPartition::sliding_window(n, 4, popts);

  DirectForecasterOptions opts;
  opts.lag.horizon = {1, 2, 4};
  opts.lag.response_lags = {1, 2, 3};
  opts.lag.predictor_lags = {{1, 2}};
  opts.fit_intercept = true;
  opts.ridge_lambda = 1e-4;

  const auto cv = DirectCVForecaster::fit(x, y, part, opts, cfg);
  const Series pred = cv.cvpredict(x, y, cfg);
  const Series avg_loss = cv.cvloss(x, y, CVLossMode::Average, RegressionLoss::MSE, cfg);
  const Series ind_loss = cv.cvloss(x, y, CVLossMode::Individual, RegressionLoss::MSE, cfg);

  expect(cv.num_folds() == 4, "expected 4 folds");
  expect(cv.num_steps() == 3, "expected 3 horizon steps");
  expect(pred.sizes().size() == 2 && pred.sizes()[0] == n && pred.sizes()[1] == 3, "expected cvpredict shape [n,3]");
  expect(avg_loss.sizes().size() == 1 && avg_loss.sizes()[0] == 3, "expected average loss shape [3]");
  expect(ind_loss.sizes().size() == 2 && ind_loss.sizes()[0] == 4 && ind_loss.sizes()[1] == 3,
         "expected individual loss shape [4,3]");

  const auto avg_vals = avg_loss.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  expect(std::all_of(avg_vals.begin(), avg_vals.end(), [](double v) { return std::isfinite(v) && v >= 0.0; }),
         "expected non-negative finite average loss values");
}

void test_overlap_and_missing_rows_behavior() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const std::int64_t n = 1200;
  const Series x = pluss::rand({n, 2}, cfg, 20260328ULL);
  const Series y = make_arx_response(x, 20260329ULL, 0.02);

  PartitionOptions popts;
  popts.direction = pluss::subsp::stat::ts::Direction::Forward;
  popts.train_size = 400;
  popts.test_size = 220;
  popts.step_size = 120;
  popts.max_lag = 6;
  const auto part = TsPartition::sliding_window(n, 5, popts);

  DirectForecasterOptions opts;
  opts.lag.horizon = {1, 3};
  opts.lag.response_lags = {1, 2};
  opts.lag.predictor_lags = {{1}};
  opts.fit_intercept = true;

  const auto cv = DirectCVForecaster::fit(x, y, part, opts, cfg);
  const Series pred = cv.cvpredict(x, y, cfg);
  const Series avg_mae = cv.cvloss(x, y, CVLossMode::Average, RegressionLoss::MAE, cfg);
  const auto pred_vals = pred.to(pluss::pCPU, ScalarType::Float64).reshape({pred.numel()}).to_vec<double>();
  const auto finite_count =
      static_cast<std::int64_t>(std::count_if(pred_vals.begin(), pred_vals.end(), [](double v) { return std::isfinite(v); }));
  expect(finite_count > 0, "expected finite predicted entries");
  expect(finite_count < pred.numel(), "expected some NaN entries for rows outside all test folds");
  expect(avg_mae.sizes()[0] == cv.num_steps(), "expected avg mae shape [num_steps]");
  expect(avg_mae.isfinite().all().item<double>() == 1.0, "expected finite mae values");
}

void test_error_paths() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const std::int64_t n = 500;
  const Series x = pluss::randn({n, 2}, cfg, 20260330ULL);
  const Series y = make_arx_response(x, 20260331ULL, 0.05);

  PartitionOptions popts;
  popts.direction = pluss::subsp::stat::ts::Direction::Forward;
  popts.train_size = 200;
  popts.test_size = 100;
  popts.step_size = 100;
  popts.max_lag = 4;
  const auto part = TsPartition::sliding_window(n, 2, popts);

  DirectForecasterOptions opts;
  opts.lag.horizon = {1, 2};
  opts.lag.response_lags = {1, 2};
  opts.lag.predictor_lags = {{1}};
  const auto cv = DirectCVForecaster::fit(x, y, part, opts, cfg);

  bool threw = false;
  try {
    (void)cv.cvpredict(x, pluss::randn({n + 1}, cfg, 77ULL), cfg);
  } catch (...) {
    threw = true;
  }
  expect(threw, "expected throw when Y size mismatches partition");

  threw = false;
  try {
    (void)cv.fold_model(-1);
  } catch (...) {
    threw = true;
  }
  expect(threw, "expected throw on invalid fold index");
}

} // namespace

int main() {
  try {
    test_cvpredict_and_losses();
    test_overlap_and_missing_rows_behavior();
    test_error_paths();
    std::cout << "direct_cv_forecaster_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "direct_cv_forecaster_test failed: " << e.what() << '\n';
    return 1;
  }
}
