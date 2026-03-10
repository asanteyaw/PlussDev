#include <pluss/ai/ml.h>

#include <cmath>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::ai::nodej::FeatureBatch;
using pluss::ai::nodej::LabeledBatch;
using pluss::ai::nodej::LinearFamilyOptions;
using pluss::ai::nodej::ModelFamily;
using pluss::ai::nodej::ModelSpec;
using pluss::ai::nodej::ObservationLayout;
using pluss::ai::nodej::StatusCode;
using pluss::ai::nodej::TaskKind;
using pluss::ai::nodej::TrainOptions;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

bool approx(double a, double b, double tol = 1e-5) {
  return std::abs(a - b) <= tol;
}

bool run_nodej_phase0_contract_checks(std::ostream* err = nullptr) {
  const Config cfg = f64();
  const Series x = pluss::make_series(std::vector<double>{
                                          -2.0, -1.0,
                                          -1.0, -0.5,
                                           1.0,  0.5,
                                           2.0,  1.0},
                                      {4, 2},
                                      cfg);
  const Series x_bad = pluss::make_series(std::vector<double>{
                                              1.0, 2.0, 3.0,
                                              2.0, 3.0, 4.0,
                                              3.0, 4.0, 5.0,
                                              4.0, 5.0, 6.0},
                                          {4, 3},
                                          cfg);
  const Series y_cls = pluss::make_series(std::vector<double>{0.0, 0.0, 1.0, 1.0}, {4}, cfg);
  const Series y_reg = pluss::make_series(std::vector<double>{-5.0, -2.5, 2.5, 5.0}, {4}, cfg);

  const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
  const FeatureBatch bad_features{.x = x_bad, .layout = ObservationLayout::Rows};
  const LabeledBatch cls_batch{.features = features, .y = y_cls};
  const LabeledBatch reg_batch{.features = features, .y = y_reg};

  if (!pluss::ai::nodej::validate_labeled(cls_batch).ok()) {
    if (err) *err << "validate_labeled should pass for classification batch\n";
    return false;
  }
  if (!pluss::ai::nodej::validate_labeled(reg_batch).ok()) {
    if (err) *err << "validate_labeled should pass for regression batch\n";
    return false;
  }

  const ModelSpec cls_spec{
      .task = TaskKind::Classification,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = pluss::ai::nodej::LinearObjective::Logistic,
          .regularization = pluss::ai::nodej::RegularizationKind::L2,
          .regularization_strength = 1e-6,
          .fit_intercept = true,
      },
  };
  auto cls_or = pluss::ai::nodej::make_classifier(cls_spec);
  if (!cls_or.ok()) {
    if (err) *err << "make_classifier should succeed for classification/linear\n";
    return false;
  }
  auto cls = cls_or.move_value();
  if (cls->task_kind() != TaskKind::Classification) {
    if (err) *err << "classifier task kind mismatch\n";
    return false;
  }
  TrainOptions train_opts{};
  train_opts.max_iterations = 5000;
  train_opts.tolerance = 1e-8;
  const auto cls_learn = cls->learn(cls_batch, train_opts);
  if (!cls_learn.ok()) {
    if (err) *err << "classifier learn should succeed for valid batch: " << cls_learn.message << "\n";
    return false;
  }

  auto cls_scores = cls->infer_scores(features);
  if (!cls_scores.ok()) {
    if (err) *err << "linear classifier infer_scores should succeed after learn\n";
    return false;
  }
  const auto score_vec = cls_scores.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  if (!(score_vec[0] < 0.0 && score_vec[1] < 0.0 && score_vec[2] > 0.0 && score_vec[3] > 0.0)) {
    if (err) *err << "linear classifier scores should separate classes by sign\n";
    return false;
  }

  auto cls_pred = cls->infer(features);
  if (!cls_pred.ok()) {
    if (err) *err << "linear classifier infer should succeed after learn\n";
    return false;
  }
  const auto pred_vec = cls_pred.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  if (!(approx(pred_vec[0], 0.0) && approx(pred_vec[1], 0.0) && approx(pred_vec[2], 1.0) && approx(pred_vec[3], 1.0))) {
    if (err) *err << "linear classifier predictions should match separable training labels\n";
    return false;
  }

  auto cls_metrics = cls->assess(cls_batch);
  if (!cls_metrics.ok()) {
    if (err) *err << "linear classifier assess should succeed\n";
    return false;
  }
  const auto cls_metric_vec = cls_metrics.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  if (cls_metric_vec.empty() || cls_metric_vec[0] < 0.99) {
    if (err) *err << "linear classifier default assess metric (accuracy) should be near perfect\n";
    return false;
  }

  auto cls_shape = cls->infer(bad_features);
  if (cls_shape.ok() || cls_shape.status().code != StatusCode::ShapeMismatch) {
    if (err) *err << "classifier infer should enforce learn-time feature count\n";
    return false;
  }

  const ModelSpec reg_tree_spec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Tree,
      .options = pluss::ai::nodej::TreeFamilyOptions{
          .criterion = pluss::ai::nodej::TreeCriterion::Mse,
          .max_depth = -1,
          .min_samples_leaf = 1,
          .min_samples_split = 2,
      },
  };
  auto reg_or = pluss::ai::nodej::make_regressor(reg_tree_spec);
  if (!reg_or.ok()) {
    if (err) *err << "make_regressor should succeed for regression/tree\n";
    return false;
  }
  auto reg = reg_or.move_value();
  if (reg->task_kind() != TaskKind::Regression) {
    if (err) *err << "regressor task kind mismatch\n";
    return false;
  }
  if (!reg->learn(reg_batch).ok()) {
    if (err) *err << "regressor learn should succeed for valid batch\n";
    return false;
  }
  auto reg_pred = reg->infer(features);
  if (!reg_pred.ok()) {
    if (err) *err << "tree regressor infer should succeed after learn\n";
    return false;
  }
  const auto reg_tree_vec = reg_pred.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  if (reg_tree_vec.size() != 4 || !std::isfinite(reg_tree_vec[0]) || !std::isfinite(reg_tree_vec[3])) {
    if (err) *err << "tree regressor predictions should be finite with expected length\n";
    return false;
  }

  const ModelSpec linear_reg_spec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = pluss::ai::nodej::LinearObjective::LeastSquares,
          .regularization = pluss::ai::nodej::RegularizationKind::L2,
          .regularization_strength = 1e-6,
          .fit_intercept = true,
      },
  };
  auto linear_reg_or = pluss::ai::nodej::make_regressor(linear_reg_spec);
  if (!linear_reg_or.ok()) {
    if (err) *err << "make_regressor should succeed for regression/linear\n";
    return false;
  }
  auto linear_reg = linear_reg_or.move_value();
  if (!linear_reg->learn(reg_batch, train_opts).ok()) {
    if (err) *err << "linear regressor learn should succeed\n";
    return false;
  }
  auto linear_reg_pred = linear_reg->infer(features);
  if (!linear_reg_pred.ok()) {
    if (err) *err << "linear regressor infer should succeed\n";
    return false;
  }
  const auto reg_vec = linear_reg_pred.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  if (!(approx(reg_vec[0], -5.0, 1e-3) && approx(reg_vec[3], 5.0, 1e-3))) {
    if (err) *err << "linear regressor predictions should match training signal\n";
    return false;
  }
  auto linear_reg_metrics = linear_reg->assess(reg_batch);
  if (!linear_reg_metrics.ok()) {
    if (err) *err << "linear regressor assess should succeed\n";
    return false;
  }
  const auto reg_metric_vec = linear_reg_metrics.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  if (reg_metric_vec.size() < 3 || reg_metric_vec[0] > 1e-6 || reg_metric_vec[2] < 0.9999) {
    if (err) *err << "linear regressor default metrics should show near-perfect fit\n";
    return false;
  }

  const ModelSpec wrong_cls_spec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{},
  };
  if (pluss::ai::nodej::make_classifier(wrong_cls_spec).ok()) {
    if (err) *err << "make_classifier should reject regression task specs\n";
    return false;
  }

  const ModelSpec wrong_reg_spec{
      .task = TaskKind::Classification,
      .family = ModelFamily::Tree,
      .options = pluss::ai::nodej::TreeFamilyOptions{},
  };
  if (pluss::ai::nodej::make_regressor(wrong_reg_spec).ok()) {
    if (err) *err << "make_regressor should reject classification task specs\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej phase0 contracts and family builders") {
  REQUIRE(run_nodej_phase0_contract_checks());
}
#else
int main() {
  if (!run_nodej_phase0_contract_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej_phase0_contract_test passed\n";
  return 0;
}
#endif
