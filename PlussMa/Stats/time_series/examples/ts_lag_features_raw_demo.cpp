#include <pluss/pluss.h>

#include <cstdint>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stat::ts;

int main() {
  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Config i64 = pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pCPU);

  const auto x = pluss::make_series(
      std::vector<double>{
          1.0, 10.0, 100.0,
          2.0, 20.0, 200.0,
          3.0, 30.0, 300.0,
          4.0, 40.0, 400.0,
          5.0, 50.0, 500.0,
          6.0, 60.0, 600.0,
      },
      {6, 3},
      f64);
  const auto y = pluss::make_series(std::vector<double>{0.1, 0.2, 0.3, 0.4, 0.5, 0.6}, {6}, f64);

  ts::LagPlanOptions opts;
  opts.horizon = {1, 2, 3};
  opts.response_lags = {1, 2};
  opts.leading_predictors = {2};
  opts.predictor_lags = {{1, 2}, {1}};
  opts.leading_predictor_lags = {{0, 1}};

  const auto plan = ts::build_lag_plan(3, opts);
  const auto design = ts::build_lagged_predictors(x, y, plan, f64);

  std::cout << "X\n" << x << "\n\n";
  std::cout << "Y\n" << y << "\n\n";
  std::cout << "horizon\n"
            << pluss::make_series(plan.horizon, {static_cast<std::int64_t>(plan.horizon.size())}, i64) << "\n\n";
  std::cout << "response_lags\n"
            << pluss::make_series(plan.response_lags, {static_cast<std::int64_t>(plan.response_lags.size())}, i64) << "\n\n";
  std::cout << "nonleading_predictor_indices\n"
            << pluss::make_series(plan.nonleading_predictor_indices,
                                  {static_cast<std::int64_t>(plan.nonleading_predictor_indices.size())},
                                  i64)
            << "\n\n";
  std::cout << "leading_predictor_indices\n"
            << pluss::make_series(plan.leading_predictor_indices,
                                  {static_cast<std::int64_t>(plan.leading_predictor_indices.size())},
                                  i64)
            << "\n\n";
  std::cout << "leading_unique_lags predictor 2\n"
            << pluss::make_series(plan.leading_unique_lags.front(),
                                  {static_cast<std::int64_t>(plan.leading_unique_lags.front().size())},
                                  i64)
            << "\n\n";
  std::cout << "max_lag=" << plan.max_lag << " max_lead=" << plan.max_lead
            << " feature_count=" << plan.feature_count << "\n\n";
  std::cout << "prepared_predictors_per_horizon\n" << plan.prepared_predictors_per_horizon << "\n\n";
  std::cout << "design_matrix\n" << design << "\n";
  return 0;
}
