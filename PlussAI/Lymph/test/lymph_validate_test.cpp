#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

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
using pluss::table::DataFrame;
using pluss::table::Thread;
using namespace pluss::ai::lymph;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

DataFrame make_frame() {
  const Series x1 = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, f64());
  const Series x2 = pluss::make_series(std::vector<double>{1000, 2000, 500, 4000, 1500, 3000, 800, 3500}, {8}, f64());
  const Series y = pluss::make_series(std::vector<double>{0, 0, 0, 1, 0, 1, 0, 1}, {8}, f64());
  return DataFrame({
      {"x1", Thread(x1)},
      {"x2", Thread(x2)},
      {"category", Thread(pluss::make_note({"a", "b", "a", "b", "a", "b", "a", "b"}))},
      {"y", Thread(y)},
  });
}

Model staged_logistic() {
  const ColumnRouter categorical = columnrouter::names({"category"});
  const ColumnRouter numeric = columnrouter::remaining_of(categorical);
  Model model = models::logistic_regression({.regularization_strength = 0.01, .fit_intercept = true});
  model.append(stage::standard_scaler(), numeric)
       .append(stage::one_hot_encoder({.max_categories = 8, .include_unknown_bucket = true}), categorical);
  return model;
}

bool run_validate_checks(std::ostream* err) {
  const DataFrame frame = make_frame();
  const DataFrameBatchOptions data_options{.label_feature = "y", .note_policy = DataNotePolicy::Factorize};

  AssessOptions assess{};
  assess.metrics = {MetricKind::Accuracy, MetricKind::F1};

  const validate::Plan plan = validate::stratified().folds(2).shuffle(false).workers(1);
  const CrossValidationReport report = plan.run(staged_logistic(), frame, data_options, TrainOptions{}, assess);
  if (report.report.folds.size() != 2 || report.report.metrics.size() != 2) {
    if (err) *err << "validate::stratified().run(model, frame, data_options, ...) should produce 2 folds x 2 metrics\n";
    return false;
  }
  if (!report.report.mean_metrics.isfinite().all().item<bool>()) {
    if (err) *err << "validate::Plan::run should produce finite mean metrics\n";
    return false;
  }

  const Series scores = plan.scores(staged_logistic(), frame, data_options, MetricKind::Accuracy);
  if (scores.numel() != 2 || !scores.isfinite().all().item<bool>()) {
    if (err) *err << "validate::Plan::scores should return one finite score per fold\n";
    return false;
  }
  const double score = plan.score(staged_logistic(), frame, data_options, MetricKind::Accuracy);
  if (!std::isfinite(score) || score < 0.0 || score > 1.0) {
    if (err) *err << "validate::Plan::score should return a finite fold mean in [0, 1]\n";
    return false;
  }

  const FrameHoldoutSplit split = validate::holdout().test_size(0.25).stratify("y").seed(11).split(frame);
  if (split.train_set.ncases() + split.test_set.ncases() != frame.ncases() || split.test_set.ncases() != 2) {
    if (err) *err << "validate::holdout().split(frame) should preserve row count with requested test size\n";
    return false;
  }

  Builder candidates = builder::build({
      {"logistic", staged_logistic()},
      {"tree", models::decision_tree_classifier({.max_depth = 3})},
  });
  const selection::CompareResult ranked = plan.run(candidates, frame, data_options, TrainOptions{}, assess,
                                                   MetricKind::Accuracy, TuneObjectivePolicy::Maximize,
                                                   selection::FitOptions{.fit_top = 1});
  if (ranked.report.trials.size() != 2 || !ranked.best().trained()) {
    if (err) *err << "validate::Plan::run(builder,...) should rank candidates and fit the winner\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("lymph validate family", "[lymph][validate]") {
  if (!run_validate_checks(&std::cerr)) {
    FAIL("lymph validate checks failed");
  }
}
#else
int main() {
  if (!run_validate_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "lymph validate checks passed\n";
  return 0;
}
#endif
