#include <pluss/ai/ml.h>
#include <pluss/mic.h>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::table::DataFrame;
using pluss::table::Thread;
using namespace pluss::ai::nodej;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

std::vector<double> as_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

DataFrame make_regression_frame() {
  const Series x0 = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0, 4.0, 5.0}, {6}, f64());
  const Series x1 = pluss::make_series(std::vector<double>{1.0, 0.0, 2.0, 1.0, 3.0, 2.0}, {6}, f64());
  const Series y = x0.mul(2.0).sub(x1.mul(0.5)).add(1.0);
  const Series w = pluss::make_series(std::vector<double>{1.0, 1.0, 0.8, 1.2, 1.0, 0.9}, {6}, f64());

  return DataFrame({
      {"x0", Thread(x0)},
      {"x1", Thread(x1)},
      {"target", Thread(y)},
      {"weight", Thread(w)},
  });
}

bool run_nodej_dataframe_bridge_checks(std::ostream* err = nullptr) {
  const DataFrame frame = make_regression_frame();

  DataFrameBatchOptions options{};
  options.feature_names = {"x0", "x1", "target", "weight"};
  options.label_feature = "target";
  options.sample_weight_feature = "weight";
  options.layout = ObservationLayout::Rows;

  auto labeled_or = labeled_batch_from_dataframe(frame, options);
  if (!labeled_or.ok()) {
    if (err) *err << "labeled_batch_from_dataframe should succeed for numeric frame\n";
    return false;
  }

  const LabeledBatch& batch = labeled_or.value();
  if (batch.features.sample_count() != 6 || batch.features.feature_count() != 2) {
    if (err) *err << "adapter should keep only requested feature columns (excluding label/weight)\n";
    return false;
  }
  if (!batch.sample_weights.defined() || batch.sample_weights.numel() != 6) {
    if (err) *err << "adapter should map optional sample weights\n";
    return false;
  }

  ModelSpec spec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-8,
          .fit_intercept = true,
      },
  };

  auto reg_or = make_regressor(spec);
  if (!reg_or.ok()) {
    if (err) *err << "make_regressor should succeed for linear regression\n";
    return false;
  }

  auto reg = reg_or.move_value();
  if (!reg->learn(batch).ok()) {
    if (err) *err << "regressor learn should succeed with DataFrame-adapted batch\n";
    return false;
  }

  auto metrics_or = reg->assess(batch, AssessOptions{.metrics = {MetricKind::Mse, MetricKind::R2}});
  if (!metrics_or.ok()) {
    if (err) *err << "regressor assess should succeed with DataFrame-adapted batch\n";
    return false;
  }

  const auto metrics = as_f64(metrics_or.value());
  if (metrics.size() != 2 || metrics[0] > 1e-9 || metrics[1] < 0.999999) {
    if (err) *err << "DataFrame-adapted linear regression should recover exact synthetic signal\n";
    return false;
  }

  DataFrameBatchOptions bad_options{};
  bad_options.label_feature = "missing_label";
  auto bad_or = labeled_batch_from_dataframe(frame, bad_options);
  if (bad_or.ok() || bad_or.status().code != StatusCode::InvalidArgument) {
    if (err) *err << "adapter should reject unknown label feature names\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej dataframe adapters from PlussMic") {
  REQUIRE(run_nodej_dataframe_bridge_checks(&std::cerr));
}
#else
int main() {
  if (!run_nodej_dataframe_bridge_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej_dataframe_bridge_test passed\n";
  return 0;
}
#endif
