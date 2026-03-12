#include <pluss/ai/ml.h>

#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using namespace pluss::ai::nodej;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

std::vector<double> as_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

bool finite_series(const Series& x) {
  return x.isfinite().all().item<bool>();
}

bool run_nodej_flow_checks(std::ostream* err = nullptr) {
  {
    Flow flow{};
    if (!flow.add_standard_scaler().ok()) {
      if (err) *err << "Flow::add_standard_scaler should succeed\n";
      return false;
    }

    const FeatureBatch features{
        .x = pluss::make_series(std::vector<double>{
                                    1.0,  2.0,
                                    2.0,  1.0,
                                    3.0, -1.0,
                                    4.0,  0.5},
                                {4, 2},
                                f64()),
        .layout = ObservationLayout::Rows,
    };

    auto before_or = flow.map(features);
    if (before_or.ok() || before_or.status().code != StatusCode::Precondition) {
      if (err) *err << "Flow::map should require learn when transform has learnables\n";
      return false;
    }

    auto learned_or = flow.learn_map(features);
    auto mapped_or = flow.map(features);
    if (!learned_or.ok() || !mapped_or.ok()) {
      if (err) *err << "Flow transform-only learn_map/map should succeed\n";
      return false;
    }
    if (learned_or.value().numel() != features.x.numel() || mapped_or.value().numel() != features.x.numel()) {
      if (err) *err << "Flow transform-only map should preserve shape\n";
      return false;
    }
    if (!finite_series(learned_or.value()) || !finite_series(mapped_or.value())) {
      if (err) *err << "Flow transform-only outputs should be finite\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -3.0, -1.0,
                                            -2.0, -1.0,
                                             std::numeric_limits<double>::quiet_NaN(), -0.8,
                                            -1.0, -0.6,
                                             1.0,  0.6,
                                             2.0,  0.8,
                                             3.0,  1.0,
                                             std::numeric_limits<double>::quiet_NaN(),  0.7},
                                        {8, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64());
    const LabeledBatch train{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    ModelSpec spec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Linear,
        .options = LinearFamilyOptions{
            .objective = LinearObjective::Logistic,
            .regularization = RegularizationKind::L2,
            .regularization_strength = 1e-6,
            .fit_intercept = true,
        },
    };

    auto flow_or = make_flow(spec);
    if (!flow_or.ok()) {
      if (err) *err << "make_flow(classifier) should succeed\n";
      return false;
    }
    Flow flow = flow_or.move_value();
    if (!flow.add_simple_imputer(prep::SimpleImputerOptions{.strategy = prep::SimpleImputerStrategy::Mean}).ok()) {
      if (err) *err << "Flow::add_simple_imputer should succeed\n";
      return false;
    }
    if (!flow.add_standard_scaler().ok()) {
      if (err) *err << "Flow::add_standard_scaler should succeed\n";
      return false;
    }

    auto infer_before_or = flow.infer(train.features);
    if (infer_before_or.ok() || infer_before_or.status().code != StatusCode::Precondition) {
      if (err) *err << "Flow::infer should require learn\n";
      return false;
    }

    TrainOptions train_options{};
    train_options.max_iterations = 3000;
    train_options.tolerance = 1e-8;
    if (!flow.learn(train, train_options).ok()) {
      if (err) *err << "Flow::learn classification should succeed\n";
      return false;
    }

    auto pred_or = flow.infer(train.features);
    auto score_or = flow.infer_scores(train.features);
    auto metric_or = flow.assess(train, AssessOptions{.metrics = {MetricKind::Accuracy, MetricKind::LogLoss}});
    if (!pred_or.ok() || !score_or.ok() || !metric_or.ok()) {
      if (err) *err << "Flow classification infer/score/assess should succeed\n";
      return false;
    }
    const auto metric = as_f64(metric_or.value());
    if (metric.size() != 2 || metric[0] < 0.75) {
      if (err) *err << "Flow classification should reach useful accuracy\n";
      return false;
    }
    if (!finite_series(score_or.value()) || !finite_series(metric_or.value())) {
      if (err) *err << "Flow classification outputs should be finite\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0,  1.0,
                                            -1.0,  std::numeric_limits<double>::quiet_NaN(),
                                             0.0,  0.0,
                                             1.0, -1.0,
                                             2.0, -2.0,
                                             3.0, -3.0},
                                        {6, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{-4.0, -1.0, 1.0, 4.0, 7.0, 10.0}, {6}, f64());
    const LabeledBatch train{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

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

    auto flow_or = make_flow(spec);
    if (!flow_or.ok()) {
      if (err) *err << "make_flow(regressor) should succeed\n";
      return false;
    }
    Flow flow = flow_or.move_value();
    if (!flow.add_simple_imputer(prep::SimpleImputerOptions{.strategy = prep::SimpleImputerStrategy::Mean}).ok()) {
      if (err) *err << "Flow::add_simple_imputer should succeed for regression\n";
      return false;
    }
    if (!flow.add_standard_scaler().ok()) {
      if (err) *err << "Flow::add_standard_scaler should succeed for regression\n";
      return false;
    }

    if (!flow.learn(train).ok()) {
      if (err) *err << "Flow::learn regression should succeed\n";
      return false;
    }

    auto pred_or = flow.infer(train.features);
    auto metrics_or = flow.assess(train, AssessOptions{.metrics = {MetricKind::Mse, MetricKind::QuantileLoss}, .regression_quantile = 0.8});
    if (!pred_or.ok() || !metrics_or.ok()) {
      if (err) *err << "Flow regression infer/assess should succeed\n";
      return false;
    }
    const auto metrics = as_f64(metrics_or.value());
    if (metrics.size() != 2 || metrics[0] > 0.6 || metrics[1] > 0.6) {
      if (err) *err << "Flow regression should produce low training losses\n";
      return false;
    }
    auto scores_or = flow.infer_scores(train.features);
    if (scores_or.ok() || scores_or.status().code != StatusCode::InvalidArgument) {
      if (err) *err << "Flow::infer_scores should reject regression flows\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej flow compose lifecycle") {
  REQUIRE(run_nodej_flow_checks(&std::cerr));
}
#else
int main() {
  if (!run_nodej_flow_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej_flow_test passed\n";
  return 0;
}
#endif
