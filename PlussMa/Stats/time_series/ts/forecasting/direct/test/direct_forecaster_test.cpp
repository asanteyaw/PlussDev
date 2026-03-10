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
using pluss::subsp::stats::ts::DirectForecaster;
using pluss::subsp::stats::ts::DirectForecasterOptions;
using pluss::subsp::stats::ts::DirectLearnerKind;
using pluss::subsp::stats::ts::RegressionLoss;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

Series make_arx_response(const Series& x, std::uint64_t seed) {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const auto x_cpu = x.to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>();
  const std::int64_t n = x.sizes()[0];
  const std::int64_t p = x.sizes()[1];
  const auto noise = pluss::randn({n}, cfg, seed).to_vec<double>();

  std::vector<double> y(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t t = 1; t < n; ++t) {
    const double x0_lag = x_cpu[static_cast<std::size_t>((t - 1) * p)];
    const double x1_lag = x_cpu[static_cast<std::size_t>((t - 1) * p + 1)];
    y[static_cast<std::size_t>(t)] =
        0.65 * y[static_cast<std::size_t>(t - 1)] + 0.30 * x0_lag - 0.20 * x1_lag + 0.02 * noise[static_cast<std::size_t>(t)];
  }
  return pluss::make_series(y, {n}, cfg);
}

void test_fit_predict_closed_loop() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const std::int64_t n = 1200;
  const Series x = pluss::randn({n, 2}, cfg, 20260312ULL);
  const Series y = make_arx_response(x, 20260313ULL);

  DirectForecasterOptions options;
  options.lag.horizon = {1};
  options.lag.response_lags = {1};
  options.lag.predictor_lags = {{1}};
  options.fit_intercept = false;
  options.learner = DirectLearnerKind::OLS;

  const auto model = DirectForecaster::fit(x, y, options, cfg);
  const Series pred = model.predict(x, y, cfg);
  const Series mse = model.loss(x, y, RegressionLoss::MSE, cfg);
  const Series prep = model.prepared_predictors(x, y, 1, cfg);
  const Series fcast = model.forecast(x, y, cfg);

  expect(model.num_steps() == 1, "expected one fitted horizon step");
  expect(pred.sizes().size() == 2 && pred.sizes()[0] == n && pred.sizes()[1] == 1, "expected prediction shape [n,1]");
  expect(prep.sizes().size() == 2 && prep.sizes()[0] == n, "expected prepared predictor rows [n,*]");
  expect(fcast.sizes().size() == 1 && fcast.sizes()[0] == 1, "expected forecast shape [1]");
  expect(mse.sizes().size() == 1 && mse.sizes()[0] == 1, "expected mse shape [1]");
  expect(mse.to_vec<double>()[0] < 0.03, "expected low in-sample mse for ARX process");
}

void test_multi_horizon_shape_and_finiteness() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const std::int64_t n = 900;
  const Series x = pluss::rand({n, 3}, cfg, 20260314ULL);
  const Series first_two = x.index_select(
      1,
      pluss::make_series(std::vector<std::int64_t>{0, 1},
                         {2},
                         pluss::Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU)));
  const Series y = make_arx_response(first_two, 20260315ULL);

  DirectForecasterOptions options;
  options.lag.horizon = {1, 2, 4};
  options.lag.response_lags = {1, 2};
  options.lag.predictor_lags = {{1, 2}};
  options.fit_intercept = true;
  options.learner = DirectLearnerKind::Ridge;
  options.ridge_lambda = 1e-5;

  const auto model = DirectForecaster::fit(x, y, options, cfg);
  const Series pred = model.predict(x, y, cfg);
  const Series mae = model.loss(x, y, RegressionLoss::MAE, cfg);

  expect(model.num_steps() == 3, "expected three fitted horizon steps");
  expect(pred.sizes().size() == 2 && pred.sizes()[0] == n && pred.sizes()[1] == 3, "expected prediction shape [n,3]");
  expect(pred.isfinite().all().item<double>() == 1.0, "expected finite predictions");
  expect(mae.sizes().size() == 1 && mae.sizes()[0] == 3, "expected mae shape [3]");
  expect(mae.isfinite().all().item<double>() == 1.0, "expected finite mae values");
}

void test_error_paths() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const std::int64_t n = 200;
  const Series x = pluss::randn({n, 2}, cfg, 20260316ULL);
  const Series y = make_arx_response(x, 20260317ULL);

  DirectForecasterOptions bad;
  bad.lag.horizon = {1};
  bad.lag.response_lags = {1};
  bad.lag.predictor_lags = {{1}};
  bad.learner = DirectLearnerKind::Ridge;
  bad.ridge_lambda = -1.0;
  bool threw = false;
  try {
    (void)DirectForecaster::fit(x, y, bad, cfg);
  } catch (...) {
    threw = true;
  }
  expect(threw, "expected fit to throw for negative ridge lambda");

  DirectForecasterOptions ok;
  ok.lag.horizon = {1};
  ok.lag.response_lags = {1};
  ok.lag.predictor_lags = {{1}};
  const auto model = DirectForecaster::fit(x, y, ok, cfg);

  threw = false;
  try {
    (void)model.predict(x, Series::undefined(), cfg);
  } catch (...) {
    threw = true;
  }
  expect(threw, "expected predict to throw without Y when response lags are required");

  DirectForecasterOptions leading;
  leading.lag.horizon = {2};
  leading.lag.response_lags = {1};
  leading.lag.leading_predictors = {1};
  leading.lag.leading_predictor_lags = {{0}};
  const auto lead_model = DirectForecaster::fit(x, y, leading, cfg);

  threw = false;
  try {
    (void)lead_model.forecast(x, y, cfg);
  } catch (...) {
    threw = true;
  }
  expect(threw, "expected forecast to throw when future leading predictors are required");
}

} // namespace

int main() {
  try {
    test_fit_predict_closed_loop();
    test_multi_horizon_shape_and_finiteness();
    test_error_paths();
    std::cout << "direct_forecaster_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "direct_forecaster_test failed: " << e.what() << '\n';
    return 1;
  }
}
