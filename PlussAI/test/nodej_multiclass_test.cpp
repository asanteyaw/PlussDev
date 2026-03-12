#include <pluss/ai/ml.h>
#include <pluss/ai/lymph.h>

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
using pluss::ai::nodej::LinearObjective;
using pluss::ai::nodej::MetricKind;
using pluss::ai::nodej::ModelFamily;
using pluss::ai::nodej::ModelSpec;
using pluss::ai::nodej::ObservationLayout;
using pluss::ai::nodej::RegularizationKind;
using pluss::ai::nodej::StatusCode;
using pluss::ai::nodej::TaskKind;
using pluss::ai::nodej::TreeCriterion;
using pluss::ai::nodej::TreeFamilyOptions;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

LabeledBatch make_three_class_batch() {
  const Series x = pluss::make_series(std::vector<double>{
                                          -3.0, -3.0,
                                          -2.5, -2.0,
                                          -2.0, -2.5,
                                          -1.8, -1.7,
                                           0.0,  3.0,
                                           0.5,  2.5,
                                          -0.4,  2.2,
                                           0.3,  1.8,
                                           3.0, -1.0,
                                           2.5, -1.5,
                                           2.2, -0.4,
                                           1.8, -0.8},
                                      {12, 2},
                                      f64());
  const Series y = pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2}, {12}, f64());
  return LabeledBatch{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y,
  };
}

LabeledBatch make_binary_batch() {
  const Series x = pluss::make_series(std::vector<double>{-2.0, -1.0, -1.0, -0.5, 1.0, 0.5, 2.0, 1.0},
                                      {4, 2},
                                      f64());
  const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 1.0, 1.0}, {4}, f64());
  return LabeledBatch{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y,
  };
}

bool approx(double a, double b, double tol = 1e-6) {
  return std::abs(a - b) <= tol;
}

bool run_nodej_multiclass_checks(std::ostream* err = nullptr) {
  const LabeledBatch tri = make_three_class_batch();

  {
    const ModelSpec spec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Linear,
        .options = LinearFamilyOptions{
            .objective = LinearObjective::Logistic,
            .regularization = RegularizationKind::L2,
            .regularization_strength = 1e-6,
            .fit_intercept = true,
        },
    };
    auto cls_or = pluss::ai::nodej::make_classifier(spec);
    if (!cls_or.ok()) {
      if (err) *err << "make_classifier should succeed for multiclass linear\n";
      return false;
    }
    auto cls = cls_or.move_value();
    if (!cls->learn(tri).ok()) {
      if (err) *err << "multiclass linear learn should succeed\n";
      return false;
    }

    auto score_or = cls->infer_scores(tri.features);
    if (!score_or.ok()) {
      if (err) *err << "multiclass linear infer_scores should succeed\n";
      return false;
    }
    const Series scores = score_or.value();
    if (scores.dim() != 2 || scores.sizes()[0] != 12 || scores.sizes()[1] != 3) {
      if (err) *err << "multiclass infer_scores should return [samples, classes]\n";
      return false;
    }

    auto pred_or = cls->infer(tri.features);
    if (!pred_or.ok()) {
      if (err) *err << "multiclass linear infer should succeed\n";
      return false;
    }
    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    const auto truth = tri.y.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    std::size_t correct = 0;
    for (std::size_t i = 0; i < pred.size(); ++i) {
      if (approx(pred[i], truth[i])) ++correct;
    }
    if (correct < 11) {
      if (err) *err << "multiclass linear should classify nearly all separable points\n";
      return false;
    }

    pluss::ai::nodej::AssessOptions assess{};
    assess.metrics = {MetricKind::Accuracy, MetricKind::BalancedAccuracy, MetricKind::Precision,
                      MetricKind::Recall, MetricKind::F1, MetricKind::LogLoss};
    auto metrics_or = cls->assess(tri, assess);
    if (!metrics_or.ok()) {
      if (err) *err << "multiclass linear assess should support multiclass metrics\n";
      return false;
    }
    const auto metrics = metrics_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (metrics.size() != 6 || metrics[0] < 0.9 || metrics[1] < 0.9 || metrics[5] > 0.4) {
      if (err) *err << "multiclass metrics should indicate high fit quality\n";
      return false;
    }

    pluss::ai::nodej::AssessOptions roc_assess{};
    roc_assess.metrics = {MetricKind::RocAuc};
    auto roc_or = cls->assess(tri, roc_assess);
    if (roc_or.ok() || roc_or.status().code != StatusCode::NotImplemented) {
      if (err) *err << "multiclass RocAuc should currently report NotImplemented\n";
      return false;
    }
  }

  {
    const ModelSpec spec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Tree,
        .options = TreeFamilyOptions{
            .criterion = TreeCriterion::Gini,
            .max_depth = 5,
            .min_samples_leaf = 1,
            .min_samples_split = 2,
        },
    };
    auto cls_or = pluss::ai::nodej::make_classifier(spec);
    if (!cls_or.ok()) {
      if (err) *err << "make_classifier should succeed for multiclass tree\n";
      return false;
    }
    auto cls = cls_or.move_value();
    if (!cls->learn(tri).ok()) {
      if (err) *err << "multiclass tree learn should succeed\n";
      return false;
    }
    auto pred_or = cls->infer(tri.features);
    if (!pred_or.ok()) {
      if (err) *err << "multiclass tree infer should succeed\n";
      return false;
    }
    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    const auto truth = tri.y.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    for (std::size_t i = 0; i < pred.size(); ++i) {
      if (!approx(pred[i], truth[i])) {
        if (err) *err << "multiclass tree should fit this training set\n";
        return false;
      }
    }
  }

  {
    const LabeledBatch binary = make_binary_batch();
    auto cls_or = pluss::ai::lymph::linear_logistic_classifier(binary, 1e-6);
    if (!cls_or.ok()) {
      if (err) *err << "lymph linear_logistic_classifier should train for binary data\n";
      return false;
    }
    auto score_or = cls_or.value()->infer_scores(binary.features);
    if (!score_or.ok()) {
      if (err) *err << "binary infer_scores should succeed\n";
      return false;
    }
    if (score_or.value().dim() != 1 || score_or.value().numel() != 4) {
      if (err) *err << "binary infer_scores shape should remain 1-D\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej multiclass ovr classification") {
  REQUIRE(run_nodej_multiclass_checks());
}
#else
int main() {
  if (!run_nodej_multiclass_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej_multiclass_test: OK\n";
  return 0;
}
#endif
