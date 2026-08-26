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

// End-to-end DataNotePolicy::Factorize check: Model must fit the
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

  models::Model model = models::logistic_regression(linear);
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
  const models::Model restored = models::Model::load(tmp.string());
  std::filesystem::remove(tmp);

  if (!restored.trained()) {
    if (err) *err << "Model::load should restore a trained model\n";
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

  models::Model model = models::logistic_regression(linear);
  TrainOptions train{};
  train.max_iterations = 2500;
  train.tolerance = 1e-8;
  model.learn_(batch, train);

  const ConfusionTable cm = reports::confusion_table(model, batch);
  if (cm.counts.dim() != 2 || cm.counts.sizes()[0] != 2 || cm.counts.sizes()[1] != 2) {
    if (err) *err << "lymph reports::confusion_table(model,batch) should return 2x2 matrix on binary data\n";
    return false;
  }

  const ClassReport report = reports::classification_summary(model, batch);
  if (report.labels.numel() != 2 || report.macro_average.numel() != 3 || report.weighted_average.numel() != 3) {
    if (err) *err << "lymph reports::classification_summary(model,batch) should expose class + aggregate metrics\n";
    return false;
  }

  const Series class_metric_values =
      reports::classification_metric_series(model, batch, AssessOptions{.metrics = {MetricKind::Accuracy, MetricKind::F1}});
  const auto metric_vec = as_f64(class_metric_values);
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

  const auto model_reports = model.reports();
  const ConfusionTable bound_cm = model_reports.confusion_table(batch);
  if (bound_cm.counts.numel() != cm.counts.numel()) {
    if (err) *err << "model.reports().confusion_table(batch) should mirror reports::confusion_table(model,batch)\n";
    return false;
  }
  const ClassReport bound_summary = model_reports.classification_summary(batch);
  if (bound_summary.macro_average.numel() != report.macro_average.numel()) {
    if (err) *err << "model.reports().classification_summary(batch) should return aggregate metrics\n";
    return false;
  }
  const Series bound_metric_series = model_reports.metric_series(batch, AssessOptions{.metrics = {MetricKind::Accuracy, MetricKind::F1}});
  if (bound_metric_series.numel() != 2 || !bound_metric_series.isfinite().all().item<bool>()) {
    if (err) *err << "model.reports().metric_series(batch) should return requested finite values\n";
    return false;
  }
  const ThresholdCurve bound_curve = model_reports.threshold_curve(batch);
  if (bound_curve.thresholds.numel() == 0 || bound_curve.f1.numel() != bound_curve.thresholds.numel()) {
    if (err) *err << "model.reports().threshold_curve(batch) should return aligned threshold/F1 vectors\n";
    return false;
  }
  const double bound_threshold = model_reports.best_threshold(batch);
  if (!std::isfinite(bound_threshold)) {
    if (err) *err << "model.reports().best_threshold(batch) should return a finite threshold\n";
    return false;
  }

  const Series predicted_labels = model.infer(batch.features);
  const auto class_metrics = metrics::classification(batch.y, predicted_labels);
  const ConfusionTable raw_cm = class_metrics.confusion_table();
  if (raw_cm.counts.numel() != cm.counts.numel()) {
    if (err) *err << "metrics::classification(y, pred).confusion_table() should mirror raw confusion_table\n";
    return false;
  }
  const MetricReport raw_class_report = class_metrics.report({MetricKind::Accuracy, MetricKind::BalancedAccuracy, MetricKind::F1});
  if (raw_class_report.values.numel() != 3 || !raw_class_report.values.isfinite().all().item<bool>()) {
    if (err) *err << "metrics::classification(y, pred).report(...) should return finite requested metrics\n";
    return false;
  }
  const auto threshold_metrics = metrics::threshold(model.infer_scores(batch.features), batch.y);
  const ThresholdCurve raw_curve = threshold_metrics.curve();
  if (raw_curve.thresholds.numel() == 0 || raw_curve.precision.numel() != raw_curve.thresholds.numel()) {
    if (err) *err << "metrics::threshold(scores, y).curve() should return aligned threshold/precision vectors\n";
    return false;
  }
  const double raw_threshold = threshold_metrics.best(MetricKind::F1);
  if (!std::isfinite(raw_threshold)) {
    if (err) *err << "metrics::threshold(scores, y).best() should return a finite threshold\n";
    return false;
  }

  const Series y_reg = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4}, f64());
  const Series p_reg = pluss::make_series(std::vector<double>{1.0, 2.5, 2.5, 4.0}, {4}, f64());
  const MetricReport reg_report = metrics::regression(y_reg, p_reg).report({MetricKind::Mse, MetricKind::Rmse, MetricKind::Mae, MetricKind::R2});
  if (reg_report.values.numel() != 4 || !reg_report.values.isfinite().all().item<bool>()) {
    if (err) *err << "metrics::regression(y, pred).report(...) should return finite requested metrics\n";
    return false;
  }

  selection::CrossValOptions cv{};
  cv.plan = CVPlanKind::CrossVal;
  cv.folds = 4;
  cv.shuffle = true;
  cv.random_state = 73;
  cv.deterministic = true;

  const selection::Plan cv_plan = selection::cross(cv);
  const Series fold_scores = cv_plan.scores(model.model_spec().value(), batch, MetricKind::Accuracy, train);
  if (fold_scores.numel() != 4 || !fold_scores.isfinite().all().item<bool>()) {
    if (err) *err << "lymph cross_validate_scores should return one finite score per fold\n";
    return false;
  }

  const double score = cv_plan.score(model.model_spec().value(), batch, MetricKind::Accuracy, train);
  if (!std::isfinite(score) || score < 0.0 || score > 1.0) {
    if (err) *err << "lymph cross_validate_score should return finite fold-mean score\n";
    return false;
  }

  AssessOptions assess{};
  assess.metrics = {MetricKind::Accuracy};
  const metrics::CrossValidationReport report_cv = cv_plan.run(model.model_spec().value(), batch, train, assess);
  const double mean_cv = report_cv.report.mean_metrics.reshape({1}).item<double>();
  if (!approx(score, mean_cv, 1e-10)) {
    if (err) *err << "lymph cross_validate_score should match cross_validate mean metric for same setup\n";
    return false;
  }

  // Reports are now produced by the reports family consuming a public Model;
  // CV still uses the spec-level validation path until validate:: lands.
  models::Model report_model = models::logistic_regression(linear);
  report_model.learn_(batch, train);
  const ConfusionTable report_cm = reports::confusion_table(report_model, batch);
  if (report_cm.counts.numel() != cm.counts.numel()) {
    if (err) *err << "reports::confusion_table(model,...) should mirror wrapper behavior\n";
    return false;
  }
  const ClassReport report_detail = reports::classification_summary(report_model, batch);
  if (report_detail.macro_average.numel() != 3) {
    if (err) *err << "reports::classification_summary(model,...) should expose aggregate metrics\n";
    return false;
  }
  const Series report_metric_series = reports::classification_metric_series(report_model, batch, AssessOptions{.metrics = {MetricKind::Accuracy}});
  if (report_metric_series.numel() <= 0) {
    if (err) *err << "reports::classification_metric_series(model,...) should produce requested metrics\n";
    return false;
  }
  const Series report_fold_scores = cv_plan.scores(report_model.model_spec().value(), batch, MetricKind::Accuracy, train);
  if (report_fold_scores.numel() != 4) {
    if (err) *err << "cross_validate_scores(model_spec,...) should emit one score per fold\n";
    return false;
  }
  const double report_score = cv_plan.score(report_model.model_spec().value(), batch, MetricKind::Accuracy, train);
  if (!std::isfinite(report_score) || report_score < 0.0 || report_score > 1.0) {
    if (err) *err << "cross_validate_score(model_spec,...) should produce finite score in [0,1]\n";
    return false;
  }

  {
    const auto triplet = models::sparse::make_triplet(6,
                                                      2,
                                                      {0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5},
                                                      {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1},
                                                      {-2.0, -1.0, -1.5, -0.7, 1.2, 1.4, 1.8, 0.9, -1.1, -0.8, 1.0, 1.2});
    const auto csr = models::sparse::to_csr(triplet);
    const Series y_bin = pluss::make_series(std::vector<double>{0.0, 0.0, 1.0, 1.0, 0.0, 1.0}, {6}, f64());

    models::sparse::BinaryLinearModel sparse_bin{};
    sparse_bin.learn_(triplet, y_bin, TrainOptions{.max_iterations = 500, .tolerance = 1e-7});
    const Series sparse_pred = sparse_bin.infer(csr);
    if (sparse_pred.numel() != 6) {
      if (err) *err << "sparse::BinaryLinearModel should infer one label per sparse sample\n";
      return false;
    }
    const auto sparse_state = sparse_bin.dump_state();
    models::sparse::BinaryLinearModel sparse_bin_copy{};
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

    const auto sparse_cv = models::sparse::cross_assess_binary(triplet, y_bin, sparse_split);
    if (static_cast<std::int64_t>(sparse_cv.folds.size()) != sparse_split.folds) {
      if (err) *err << "sparse::cross_assess_binary should produce one report per fold\n";
      return false;
    }

    const auto sparse_fold_report = models::sparse::assess_binary_folds(triplet, y_bin, sparse_split);
    if (static_cast<std::int64_t>(sparse_fold_report.folds.size()) != sparse_split.folds) {
      if (err) *err << "sparse::assess_binary_folds should produce one report per fold\n";
      return false;
    }

    const LinearFamilyOptions linear_sparse = linear;
    const auto sparse_tune = models::sparse::tune_binary({linear_sparse}, triplet, y_bin, sparse_split);
    if (sparse_tune.trials.empty()) {
      if (err) *err << "sparse::tune_binary should return at least one trial\n";
      return false;
    }
    if (!models::sparse::has_best_trial(sparse_tune)) {
      if (err) *err << "sparse::tune_binary should produce a best trial index\n";
      return false;
    }
    const auto& sparse_best = models::sparse::best_trial(sparse_tune);
    (void)sparse_best;
    const Series sparse_obj = models::sparse::objective_series(sparse_tune);
    const Series sparse_rank = models::sparse::objective_rank_series(sparse_tune);
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
TEST_CASE("lymph Model DataNotePolicy::Factorize", "[lymph][factorize]") {
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
