#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
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

DataFrame make_factorize_train_frame() {
  const Series x0 = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0}, {8}, f64());
  const Series x1 = pluss::make_series(std::vector<double>{0.1, 0.2, 0.1, 0.3, 0.2, 0.1, 0.3, 0.2}, {8}, f64());
  const auto color = pluss::make_note({"red", "green", "blue", "red", "green", "blue", "red", "green"});
  const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64());
  return DataFrame({
      {"x0", Thread(x0)},
      {"x1", Thread(x1)},
      {"color", Thread(color)},
      {"target", Thread(y)},
  });
}

DataFrame make_factorize_known_frame() {
  const Series x0 = pluss::make_series(std::vector<double>{1.0, 4.0, 6.0}, {3}, f64());
  const Series x1 = pluss::make_series(std::vector<double>{0.2, 0.2, 0.1}, {3}, f64());
  const auto color = pluss::make_note({"blue", "green", "red"});
  return DataFrame({
      {"x0", Thread(x0)},
      {"x1", Thread(x1)},
      {"color", Thread(color)},
  });
}

DataFrame make_factorize_unseen_frame() {
  const Series x0 = pluss::make_series(std::vector<double>{1.0, 4.0, 6.0}, {3}, f64());
  const Series x1 = pluss::make_series(std::vector<double>{0.2, 0.2, 0.1}, {3}, f64());
  const auto color = pluss::make_note({"blue", "purple", "red"});
  return DataFrame({
      {"x0", Thread(x0)},
      {"x1", Thread(x1)},
      {"color", Thread(color)},
  });
}

// End-to-end DataNotePolicy::Factorize check: WorkflowModel must fit the
// raw-text "color" column's category map once at learn_() time, reuse
// (never refit) that frozen map on every later infer() call, and carry it
// through save()/load() unchanged.
bool run_lymph_factorize_checks(std::ostream* err = nullptr) {
  const DataFrame train_frame = make_factorize_train_frame();

  DataFrameBatchOptions data_options{};
  data_options.feature_names = {"x0", "x1", "color"};
  data_options.label_feature = "target";
  data_options.note_policy = DataNotePolicy::Factorize;

  LinearFamilyOptions linear{};
  linear.objective = LinearObjective::Logistic;
  linear.regularization = RegularizationKind::L2;
  linear.regularization_strength = 1e-6;
  linear.fit_intercept = true;

  WorkflowModel model = linear_factory::logistic_regression(linear);
  TrainOptions train{};
  train.max_iterations = 2000;
  train.tolerance = 1e-8;
  model.learn_(train_frame, data_options, train);

  if (!model.trained()) {
    if (err) *err << "Factorize learn_(frame,...) should leave the model trained\n";
    return false;
  }

  // The train-time category map is {blue, green, red}; "purple" must be
  // rejected under UnknownCategoryPolicy::Error at infer time -- if infer()
  // were refitting fresh codes per call instead of reusing the frozen map,
  // "purple" would just get accepted as a legitimate new category.
  DataFrameBatchOptions error_options = data_options;
  error_options.factorize_unknown_categorical = UnknownCategoryPolicy::Error;
  bool threw = false;
  try {
    (void)model.infer(make_factorize_unseen_frame(), error_options);
  } catch (...) {
    threw = true;
  }
  if (!threw) {
    if (err) *err << "infer() on an unseen category with UnknownCategoryPolicy::Error should throw\n";
    return false;
  }

  const DataFrame known_frame = make_factorize_known_frame();
  const Series predicted_before = model.infer(known_frame, data_options);
  const auto predicted_before_vec = as_f64(predicted_before);
  if (predicted_before_vec.size() != 3) {
    if (err) *err << "infer() on known categories should produce one prediction per row\n";
    return false;
  }

  const std::filesystem::path tmp = std::filesystem::temp_directory_path() / "lymph_factorize_roundtrip.bundle";
  const LabeledBatch train_batch_for_save =
      resolve(labeled_batch_from_dataframe(train_frame, data_options), "test.labeled_batch_from_dataframe(save)");
  model.save(tmp.string(), train_batch_for_save, train);
  const WorkflowModel restored = WorkflowModel::load(tmp.string());
  std::filesystem::remove(tmp);

  if (!restored.trained()) {
    if (err) *err << "WorkflowModel::load should restore a trained model\n";
    return false;
  }

  const auto predicted_after_vec = as_f64(restored.infer(known_frame, data_options));
  if (predicted_after_vec != predicted_before_vec) {
    if (err) *err << "infer() predictions on known categories should be identical before/after save/load\n";
    return false;
  }

  // The restored model's category map must still be frozen at the
  // train-time set, not silently widened/reset by the round trip.
  bool threw_after_reload = false;
  try {
    (void)restored.infer(make_factorize_unseen_frame(), error_options);
  } catch (...) {
    threw_after_reload = true;
  }
  if (!threw_after_reload) {
    if (err) *err << "restored model should still reject an unseen category after save/load\n";
    return false;
  }

  return true;
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

  const ConfusionTable cm = confusion_table(model, batch);
  if (cm.counts.dim() != 2 || cm.counts.sizes()[0] != 2 || cm.counts.sizes()[1] != 2) {
    if (err) *err << "lymph confusion_table(model,batch) should return 2x2 matrix on binary data\n";
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
  cv.plan = CVPlanKind::CrossVal;
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

  // Was routed through the old Workflow facade (make_pipeline(...) wrapping
  // a WorkflowModel just to call confusion_table/classification_summary/
  // classification_metric_series/cross_validate_scores/cross_validate_score
  // on it) -- every one of those already has a WorkflowModel overload
  // directly (confirmed in workflow_model.h/metrics.h before dropping the
  // wrapper), so calling them on wf_model itself is the same operation with
  // one less layer of indirection.
  WorkflowModel wf_model = linear_factory::logistic_regression(linear);
  wf_model.learn_(batch, train);
  const ConfusionTable wf_cm = confusion_table(wf_model, batch);
  if (wf_cm.counts.numel() != cm.counts.numel()) {
    if (err) *err << "confusion_table(model,...) should mirror wrapper behavior\n";
    return false;
  }
  const ClassReport wf_report = classification_summary(wf_model, batch);
  if (wf_report.macro_average.numel() != 3) {
    if (err) *err << "classification_summary(model,...) should expose aggregate metrics\n";
    return false;
  }
  const Series wf_metric_series = classification_metric_series(wf_model, batch, AssessOptions{.metrics = {MetricKind::Accuracy}});
  if (wf_metric_series.numel() <= 0) {
    if (err) *err << "classification_metric_series(model,...) should produce requested metrics\n";
    return false;
  }
  const Series wf_fold_scores = cross_validate_scores(wf_model, batch, MetricKind::Accuracy, cv, train);
  if (wf_fold_scores.numel() != 4) {
    if (err) *err << "cross_validate_scores(model,...) should emit one score per fold\n";
    return false;
  }
  const double wf_score = cross_validate_score(wf_model, batch, MetricKind::Accuracy, cv, train);
  if (!std::isfinite(wf_score) || wf_score < 0.0 || wf_score > 1.0) {
    if (err) *err << "cross_validate_score(model,...) should produce finite score in [0,1]\n";
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
    sparse_split.plan = CVPlanKind::CrossVal;
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
TEST_CASE("lymph WorkflowModel DataNotePolicy::Factorize", "[lymph][factorize]") {
  if (!run_lymph_factorize_checks(&std::cerr)) {
    FAIL("lymph factorize checks failed");
  }
}
#else
int main() {
  if (!run_lymph_metrics_checks(&std::cerr)) {
    return 1;
  }
  if (!run_lymph_factorize_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "lymph metrics checks passed\n";
  return 0;
}
#endif
