#include <pluss/ai/ml.h>

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

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

bool run_nodej_phase0_contract_checks(std::ostream* err = nullptr) {
  const Config cfg = f64();
  const Series x = pluss::make_series(std::vector<double>{
                                          1.0, 2.0,
                                          2.0, 3.0,
                                          3.0, 4.0,
                                          4.0, 5.0},
                                      {4, 2},
                                      cfg);
  const Series x_bad = pluss::make_series(std::vector<double>{
                                              1.0, 2.0, 3.0,
                                              2.0, 3.0, 4.0,
                                              3.0, 4.0, 5.0,
                                              4.0, 5.0, 6.0},
                                          {4, 3},
                                          cfg);
  const Series y_cls = pluss::make_series(std::vector<double>{0.0, 1.0, 0.0, 1.0}, {4}, cfg);
  const Series y_reg = pluss::make_series(std::vector<double>{1.2, 2.4, 3.6, 4.8}, {4}, cfg);

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
      .options = LinearFamilyOptions{},
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
  if (!cls->train(cls_batch).ok()) {
    if (err) *err << "classifier train should succeed for valid batch\n";
    return false;
  }

  auto cls_scores = cls->infer_scores(features);
  if (cls_scores.ok() || cls_scores.status().code != StatusCode::NotImplemented) {
    if (err) *err << "Phase 0 infer_scores should fail fast with NotImplemented\n";
    return false;
  }
  auto cls_pred = cls->infer(features);
  if (cls_pred.ok() || cls_pred.status().code != StatusCode::NotImplemented) {
    if (err) *err << "Phase 0 infer should fail fast with NotImplemented\n";
    return false;
  }
  auto cls_shape = cls->infer(bad_features);
  if (cls_shape.ok() || cls_shape.status().code != StatusCode::ShapeMismatch) {
    if (err) *err << "classifier infer should enforce training-time feature count\n";
    return false;
  }

  const ModelSpec reg_spec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Tree,
      .options = pluss::ai::nodej::TreeFamilyOptions{},
  };
  auto reg_or = pluss::ai::nodej::make_regressor(reg_spec);
  if (!reg_or.ok()) {
    if (err) *err << "make_regressor should succeed for regression/tree\n";
    return false;
  }
  auto reg = reg_or.move_value();
  if (reg->task_kind() != TaskKind::Regression) {
    if (err) *err << "regressor task kind mismatch\n";
    return false;
  }
  if (!reg->train(reg_batch).ok()) {
    if (err) *err << "regressor train should succeed for valid batch\n";
    return false;
  }
  auto reg_pred = reg->infer(features);
  if (reg_pred.ok() || reg_pred.status().code != StatusCode::NotImplemented) {
    if (err) *err << "Phase 0 regressor infer should fail fast with NotImplemented\n";
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
