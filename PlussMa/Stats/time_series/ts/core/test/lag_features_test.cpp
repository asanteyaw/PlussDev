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
using pluss::subsp::stats::ts::LagPlanOptions;
using pluss::subsp::stats::ts::build_lag_plan;
using pluss::subsp::stats::ts::build_lagged_predictors;

std::vector<double> to_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).host<double>();
}

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void test_plan_shape_and_mask() {
  LagPlanOptions options;
  options.horizon = {1, 2, 3};
  options.response_lags = {1};
  options.leading_predictors = {2};

  const auto plan = build_lag_plan(3, options);
  expect(plan.max_lag == 1, "expected max_lag=1");
  expect(plan.max_lead == 2, "expected max_lead=2");
  expect(plan.feature_count == 6, "expected feature_count=6");
  expect(plan.nonleading_predictor_indices.size() == 2, "expected 2 nonleading predictors");
  expect(plan.leading_predictor_indices.size() == 1, "expected 1 leading predictor");
  expect(plan.leading_unique_lags.size() == 1, "expected 1 leading lag group");
  expect(plan.leading_unique_lags[0].size() == 3, "expected 3 unique leading shifts");
  expect(plan.leading_unique_lags[0][0] == -2, "expected leading shift -2");
  expect(plan.leading_unique_lags[0][1] == -1, "expected leading shift -1");
  expect(plan.leading_unique_lags[0][2] == 0, "expected leading shift 0");

  const auto mask = plan.prepared_predictors_per_horizon.to(pluss::pCPU, ScalarType::Bool);
  const auto mask_sizes = mask.sizes();
  expect(mask_sizes.size() == 2 && mask_sizes[0] == 3 && mask_sizes[1] == 6, "expected prepared mask shape [3,6]");
  const auto vals = mask.reshape({mask.numel()}).host<std::uint8_t>();
  const std::vector<std::uint8_t> expected{
      1, 1, 1, 0, 0, 1,
      1, 1, 1, 0, 1, 0,
      1, 1, 1, 1, 0, 0};
  expect(vals == expected, "prepared_predictors_per_horizon mismatch");
}

void test_lagged_predictor_construction() {
  LagPlanOptions options;
  options.horizon = {1, 2, 3};
  options.response_lags = {1};
  options.leading_predictors = {2};
  const auto plan = build_lag_plan(3, options);

  const pluss::Config cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series x = pluss::make_series(std::vector<double>{
                                          1, 10, 100,
                                          2, 20, 200,
                                          3, 30, 300,
                                          4, 40, 400,
                                          5, 50, 500},
                                      {5, 3},
                                      cfg);
  const Series y = pluss::make_series(std::vector<double>{7, 8, 9, 10, 11}, {5}, cfg);

  const Series design = build_lagged_predictors(x, y, plan, cfg);
  const auto design_sizes = design.sizes();
  expect(design_sizes.size() == 2 && design_sizes[0] == 8 && design_sizes[1] == 6, "expected design shape [8,6]");

  const auto col0 = to_f64(design.select(1, 0));
  expect(std::isnan(col0[0]) && std::isnan(col0[1]) && std::isnan(col0[2]), "nonleading lag col should start with NaNs");
  expect(col0[3] == 1.0 && col0[4] == 2.0 && col0[5] == 3.0 && col0[6] == 4.0 && col0[7] == 5.0,
         "nonleading lag col values mismatch");

  const auto col2 = to_f64(design.select(1, 2));
  expect(std::isnan(col2[0]) && std::isnan(col2[1]) && std::isnan(col2[2]), "response lag col should start with NaNs");
  expect(col2[3] == 7.0 && col2[4] == 8.0 && col2[5] == 9.0 && col2[6] == 10.0 && col2[7] == 11.0,
         "response lag col values mismatch");

  const auto lead_lag2 = to_f64(design.select(1, 3));
  const auto lead_lag1 = to_f64(design.select(1, 4));
  const auto lead_step1 = to_f64(design.select(1, 5));
  expect(lead_lag2[0] == 100.0 && lead_lag2[1] == 200.0 && lead_lag2[2] == 300.0 && lead_lag2[3] == 400.0 &&
             lead_lag2[4] == 500.0 && std::isnan(lead_lag2[5]) && std::isnan(lead_lag2[6]) && std::isnan(lead_lag2[7]),
         "leading lag2 values mismatch");
  expect(std::isnan(lead_lag1[0]) && lead_lag1[1] == 100.0 && lead_lag1[2] == 200.0 && lead_lag1[3] == 300.0 &&
             lead_lag1[4] == 400.0 && lead_lag1[5] == 500.0,
         "leading lag1 values mismatch");
  expect(lead_step1[2] == 100.0 && lead_step1[3] == 200.0 && lead_step1[4] == 300.0 && lead_step1[5] == 400.0 &&
             lead_step1[6] == 500.0 && std::isnan(lead_step1[0]) && std::isnan(lead_step1[1]) && std::isnan(lead_step1[7]),
         "leading step1 values mismatch");
}

} // namespace

int main() {
  try {
    test_plan_shape_and_mask();
    test_lagged_predictor_construction();
    std::cout << "lag_features_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "lag_features_test failed: " << e.what() << '\n';
    return 1;
  }
}
