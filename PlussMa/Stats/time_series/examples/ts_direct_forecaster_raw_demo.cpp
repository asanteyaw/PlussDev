#include <pluss/ma.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const std::int64_t n = 400;
  const auto x = pluss::series::randn({n, 2}, cfg, 20260324ULL);
  const auto x_cpu = x.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().host<double>();
  const auto noise = pluss::series::randn({n}, cfg, 20260325ULL).host<double>();

  std::vector<double> yv(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t t = 1; t < n; ++t) {
    const double x0_lag = x_cpu[static_cast<std::size_t>((t - 1) * 2)];
    const double x1_lag = x_cpu[static_cast<std::size_t>((t - 1) * 2 + 1)];
    yv[static_cast<std::size_t>(t)] =
        0.70 * yv[static_cast<std::size_t>(t - 1)] + 0.25 * x0_lag - 0.10 * x1_lag + 0.03 * noise[static_cast<std::size_t>(t)];
  }
  const auto y = pluss::make_series(yv, {n}, cfg);

  pluss::subsp::stats::ts::DirectForecasterOptions options;
  options.lag.horizon = {1, 2, 3};
  options.lag.response_lags = {1, 2};
  options.lag.predictor_lags = {{1, 2}};
  options.fit_intercept = true;
  options.learner = pluss::subsp::stats::ts::DirectLearnerKind::Ridge;
  options.ridge_lambda = 1e-4;

  const auto model = pluss::subsp::stats::ts::DirectForecaster::fit(x, y, options, cfg);
  const auto pred = model.predict(x, y, cfg);
  const auto mse = model.loss(x, y, pluss::subsp::stats::ts::RegressionLoss::MSE, cfg);
  const auto mae = model.loss(x, y, pluss::subsp::stats::ts::RegressionLoss::MAE, cfg);
  const auto prep_h2 = model.prepared_predictors(x, y, 2, cfg);
  const auto fcast = model.forecast(x, y, cfg);

  std::cout << "DirectForecaster\n";
  std::cout << "horizon=" << pluss::make_series(model.lag_plan().horizon,
                                                 {static_cast<std::int64_t>(model.lag_plan().horizon.size())},
                                                 pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pCPU))
            << "\n";
  std::cout << "max_lag=" << model.lag_plan().max_lag << " max_lead=" << model.lag_plan().max_lead
            << " feature_count=" << model.lag_plan().feature_count << "\n";
  std::cout << "prepared_predictors_per_horizon\n" << model.lag_plan().prepared_predictors_per_horizon << "\n\n";

  for (std::int64_t i = 0; i < model.num_steps(); ++i) {
    const auto& step = model.step(i);
    std::cout << "step[" << i << "] horizon=" << step.horizon << "\n";
    std::cout << "active_columns="
              << pluss::make_series(step.active_columns,
                                    {static_cast<std::int64_t>(step.active_columns.size())},
                                    pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pCPU))
              << "\n";
    std::cout << "beta=" << step.beta << "\n\n";
  }

  std::cout << "pred\n" << pred << "\n\n";
  std::cout << "prepared_predictors(h=2)\n" << prep_h2 << "\n\n";
  std::cout << "forecast\n" << fcast << "\n\n";
  std::cout << "mse_per_horizon\n" << mse << "\n\n";
  std::cout << "mae_per_horizon\n" << mae << "\n";
  return 0;
}
