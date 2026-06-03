#include <pluss/ai/lymph.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using namespace pluss::ai::lymph;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

bool approx(double a, double b, double tol = 1e-8) {
  return std::abs(a - b) <= tol;
}

std::vector<double> as_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

LabeledBatch make_xor_batch() {
  const Series x = pluss::make_series(std::vector<double>{
                                          -1.0, -1.0,
                                          -1.0,  1.0,
                                           1.0, -1.0,
                                           1.0,  1.0,
                                          -0.9, -0.9,
                                          -0.9,  0.9,
                                           0.9, -0.9,
                                           0.9,  0.9,
                                          -1.2, -1.1,
                                          -1.1,  1.2,
                                           1.1, -1.2,
                                           1.2,  1.1,
                                          -0.4, -0.7,
                                          -0.5,  0.8,
                                           0.6, -0.8,
                                           0.5,  0.7},
                                      {16, 2},
                                      f64());
  const Series y = pluss::make_series(std::vector<double>{
                                          0.0, 1.0, 1.0, 0.0,
                                          0.0, 1.0, 1.0, 0.0,
                                          0.0, 1.0, 1.0, 0.0,
                                          0.0, 1.0, 1.0, 0.0},
                                      {16},
                                      f64());
  return LabeledBatch{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y,
  };
}

bool run_lymph_metrics_checks(std::ostream* err = nullptr) {
  const LabeledBatch batch = make_xor_batch();

  LinearFamilyOptions linear{};
  linear.objective = LinearObjective::Logistic;
  linear.regularization = RegularizationKind::L2;
  linear.regularization_strength = 1e-6;
  linear.fit_intercept = true;

  WorkflowModel model = linear_factory::logistic_regression(linear);
  TrainOptions train{};
  train.max_iterations = 2500;
  train.tolerance = 1e-8;
  model.learn_(batch, train);

  const ConfusionTable cm = confusion_matrix(model, batch);
  if (cm.counts.dim() != 2 || cm.counts.sizes()[0] != 2 || cm.counts.sizes()[1] != 2) {
    if (err) *err << "lymph confusion_matrix(model,batch) should return 2x2 matrix on binary data\n";
    return false;
  }

  const ClassReport report = classification_summary(model, batch);
  if (report.labels.numel() != 2 || report.macro_average.numel() != 3 || report.weighted_average.numel() != 3) {
    if (err) *err << "lymph classification_summary(model,batch) should expose class + aggregate metrics\n";
    return false;
  }

  const Series metrics = classification_metric_series(model, batch, AssessOptions{.metrics = {MetricKind::Accuracy, MetricKind::F1}});
  const auto metric_vec = as_f64(metrics);
  if (metric_vec.empty()) {
    if (err) *err << "lymph classification_metric_series(model,batch) should return requested assess metrics\n";
    return false;
  }
  for (double value : metric_vec) {
    if (!std::isfinite(value)) {
      if (err) *err << "lymph classification_metric_series(model,batch) should return finite values\n";
      return false;
    }
  }

  CrossValidationOptions cv{};
  cv.plan = CVPlanKind::KFold;
  cv.folds = 4;
  cv.shuffle = true;
  cv.random_state = 73;
  cv.deterministic = true;

  const Series fold_scores = cross_validate_scores(model.workflow().model_spec().value(), batch, MetricKind::Accuracy, cv, train);
  if (fold_scores.numel() != 4 || !fold_scores.isfinite().all().item<bool>()) {
    if (err) *err << "lymph cross_validate_scores should return one finite score per fold\n";
    return false;
  }

  const double score = cross_validate_score(model.workflow().model_spec().value(), batch, MetricKind::Accuracy, cv, train);
  if (!std::isfinite(score) || score < 0.0 || score > 1.0) {
    if (err) *err << "lymph cross_validate_score should return finite fold-mean score\n";
    return false;
  }

  AssessOptions assess{};
  assess.metrics = {MetricKind::Accuracy};
  const CrossValidationReport report_cv = cross_validate(model.workflow().model_spec().value(), batch, cv, train, assess);
  const double mean_cv = report_cv.report.mean_metrics.reshape({1}).item<double>();
  if (!approx(score, mean_cv, 1e-10)) {
    if (err) *err << "lymph cross_validate_score should match cross_validate mean metric for same setup\n";
    return false;
  }

  WorkflowModel wf_model = linear_factory::logistic_regression(linear);
  wf_model.learn_(batch, train);
  pluss::ai::lymph::Workflow wf = make_pipeline(std::move(wf_model));
  const ConfusionTable wf_cm = wf.confusion_matrix(batch);
  if (wf_cm.counts.numel() != cm.counts.numel()) {
    if (err) *err << "Workflow::confusion_matrix should mirror WorkflowModel wrapper behavior\n";
    return false;
  }
  const ClassReport wf_report = wf.classification_summary(batch);
  if (wf_report.macro_average.numel() != 3) {
    if (err) *err << "Workflow::classification_summary should expose aggregate metrics\n";
    return false;
  }
  const Series wf_metric_series = wf.classification_metric_series(batch, AssessOptions{.metrics = {MetricKind::Accuracy}});
  if (wf_metric_series.numel() <= 0) {
    if (err) *err << "Workflow::classification_metric_series should produce requested metrics\n";
    return false;
  }
  const Series wf_fold_scores = cross_validate_scores(wf, batch, MetricKind::Accuracy, cv, train);
  if (wf_fold_scores.numel() != 4) {
    if (err) *err << "cross_validate_scores(workflow,...) should emit one score per fold\n";
    return false;
  }
  const double wf_score = cross_validate_score(wf, batch, MetricKind::Accuracy, cv, train);
  if (!std::isfinite(wf_score) || wf_score < 0.0 || wf_score > 1.0) {
    if (err) *err << "cross_validate_score(workflow,...) should produce finite score in [0,1]\n";
    return false;
  }

  {
    const auto triplet = sparse::make_triplet(6,
                                              2,
                                              {0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5},
                                              {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1},
                                              {-2.0, -1.0, -1.5, -0.7, 1.2, 1.4, 1.8, 0.9, -1.1, -0.8, 1.0, 1.2});
    const auto csr = sparse::to_csr(triplet);
    const Series y_bin = pluss::make_series(std::vector<double>{0.0, 0.0, 1.0, 1.0, 0.0, 1.0}, {6}, f64());

    sparse::BinaryLinearModel sparse_bin{};
    sparse_bin.learn_(triplet, y_bin, TrainOptions{.max_iterations = 500, .tolerance = 1e-7});
    const Series sparse_pred = sparse_bin.infer(csr);
    if (sparse_pred.numel() != 6) {
      if (err) *err << "sparse::BinaryLinearModel should infer one label per sparse sample\n";
      return false;
    }
    const auto sparse_state = sparse_bin.dump_state();
    sparse::BinaryLinearModel sparse_bin_copy{};
    sparse_bin_copy.load_state_(pluss::msharp::RangeView::Series(sparse_state));
    const Series sparse_pred_copy = sparse_bin_copy.infer(triplet);
    if (sparse_pred_copy.numel() != 6) {
      if (err) *err << "sparse::BinaryLinearModel load_state_ should restore inference path\n";
      return false;
    }

    SplitOptions sparse_split{};
    sparse_split.plan = CVPlanKind::KFold;
    sparse_split.folds = 3;
    sparse_split.shuffle = true;
    sparse_split.random.seed = 17;
    sparse_split.random.deterministic = true;

    const auto sparse_cv = sparse::cross_assess_binary(triplet, y_bin, sparse_split);
    if (static_cast<std::int64_t>(sparse_cv.folds.size()) != sparse_split.folds) {
      if (err) *err << "sparse::cross_assess_binary should produce one report per fold\n";
      return false;
    }

    const auto sparse_fold_report = sparse::assess_binary_folds(triplet, y_bin, sparse_split);
    if (static_cast<std::int64_t>(sparse_fold_report.folds.size()) != sparse_split.folds) {
      if (err) *err << "sparse::assess_binary_folds should produce one report per fold\n";
      return false;
    }

    const LinearFamilyOptions linear_sparse = linear;
    const auto sparse_tune = sparse::tune_binary({linear_sparse}, triplet, y_bin, sparse_split);
    if (sparse_tune.trials.empty()) {
      if (err) *err << "sparse::tune_binary should return at least one trial\n";
      return false;
    }
    if (!sparse::has_best_trial(sparse_tune)) {
      if (err) *err << "sparse::tune_binary should produce a best trial index\n";
      return false;
    }
    const auto& sparse_best = sparse::best_trial(sparse_tune);
    (void)sparse_best;
    const Series sparse_obj = sparse::objective_series(sparse_tune);
    const Series sparse_rank = sparse::objective_rank_series(sparse_tune);
    if (sparse_obj.numel() != static_cast<std::int64_t>(sparse_tune.trials.size())
        || sparse_rank.numel() != static_cast<std::int64_t>(sparse_tune.objective_rank.size())) {
      if (err) *err << "sparse tune objective artifact helpers should mirror trial and rank sizes\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("lymph metrics wrappers", "[lymph][metrics]") {
  if (!run_lymph_metrics_checks(&std::cerr)) {
    FAIL("lymph metrics checks failed");
  }
}
#else
int main() {
  if (!run_lymph_metrics_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "lymph metrics checks passed\n";
  return 0;
}
#endif
