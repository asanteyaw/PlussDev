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
  const Series timestamp = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, f64());
  const Series x1 = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, f64());
  const Series x2 = pluss::make_series(std::vector<double>{1000, 2000, 500, 4000, 1500, 3000, 800, 3500}, {8}, f64());
  const Series y = pluss::make_series(std::vector<double>{0, 0, 0, 1, 0, 1, 0, 1}, {8}, f64());
  return DataFrame({
      {"timestamp", Thread(timestamp)},
      {"x1", Thread(x1)},
      {"x2", Thread(x2)},
      {"category", Thread(pluss::make_note({"a", "b", "a", "b", "a", "b", "a", "b"}))},
      {"y", Thread(y)},
  });
}

Model staged_logistic() {
  const ColumnRouter categorical = columnrouter::names({"category"});
  const ColumnRouter numeric = columnrouter::remaining_of(categorical);
  Model model = models::decision_tree_classifier({.max_depth = 3});
  model.append(stage::standard_scaler(), numeric)
       .append(stage::one_hot_encoder({.max_categories = 8, .include_unknown_bucket = true}), categorical);
  return model;
}

bool run_validate_checks(std::ostream* err) {
  const DataFrame frame = make_frame();
  const DataFrameBatchOptions data_options{
      .feature_names = {"x1", "x2", "category"},
      .label_feature = "y",
      .note_policy = DataNotePolicy::Factorize,
  };

  AssessOptions assess{};
  assess.metrics = {MetricKind::Accuracy, MetricKind::F1};
  TrainOptions quick_train{};
  quick_train.max_iterations = 100;
  quick_train.tolerance = 1e-4;

  const validate::Plan plan = validate::stratified().folds(2).shuffle(false).workers(1);
  const CrossValidationReport report = plan.run(staged_logistic(), frame, data_options, quick_train, assess);
  if (report.report.folds.size() != 2 || report.report.metrics.size() != 2) {
    if (err) *err << "validate::stratified().run(model, frame, data_options, ...) should produce 2 folds x 2 metrics\n";
    return false;
  }
  if (report.report.mean_metrics.numel() != 2) {
    if (err) *err << "validate::Plan::run should produce one mean value per requested metric\n";
    return false;
  }

  const Series scores = plan.scores(staged_logistic(), frame, data_options, MetricKind::Accuracy, quick_train);
  if (scores.numel() != 2) {
    if (err) *err << "validate::Plan::scores should return one score per fold\n";
    return false;
  }
  const prep::HoldoutSplit split = validate::holdout().test_size(0.25).stratify("y").seed(11).split(frame);
  if (!split.has_frame() || split.has_batch()
      || split.train_set.ncases() + split.test_set.ncases() != frame.ncases()
      || split.test_set.ncases() != 2) {
    if (err) *err << "validate::holdout().split(frame) should preserve row count with requested test size\n";
    return false;
  }

  const LabeledBatch batch = resolve(labeled_batch_from_dataframe(frame, data_options), "test.labeled_batch_from_dataframe");
  const prep::HoldoutSplit batch_split = validate::holdout().test_size(0.25).stratify(batch.y).seed(11).split(batch);
  if (batch_split.has_frame() || !batch_split.has_batch()
      || batch_split.train_batch.features.sample_count() + batch_split.test_batch.features.sample_count()
             != batch.features.sample_count()
      || batch_split.test_batch.features.sample_count() != 2) {
    if (err) *err << "validate::holdout().split(batch) should return the same HoldoutSplit type with batch fields populated\n";
    return false;
  }

  Builder candidates = builder::build({
      {"staged_tree", staged_logistic()},
      {"plain_tree", models::decision_tree_classifier({.max_depth = 2})},
  });
  const selection::CompareResult ranked = plan.run(candidates, frame, data_options, quick_train, assess,
                                                   MetricKind::Accuracy, TuneObjectivePolicy::Maximize,
                                                   selection::FitOptions{.fit_top = 1});
  if (ranked.report.trials.size() != 2 || !ranked.best().trained()) {
    if (err) *err << "validate::Plan::run(builder,...) should rank candidates and fit the winner\n";
    return false;
  }

  Payload time_payload = payload::make(frame)
      .features({"x1", "x2", "category"})
      .label("y")
      .time(TimeOptions{.index = "timestamp", .horizon = 1, .gap = 1, .lookback = 4})
      .note(DataNotePolicy::Factorize);

  Policy gate{.require_columns = {"timestamp", "x1", "x2", "category", "y"},
              .reject_missing = {"timestamp", "y"},
              .allow_missing = {"x1", "x2", "category"}};

  Workflow time_workflow = workflow::make(models::linear_regression(), gate)
      .time(TimeOptions{.index = "timestamp", .horizon = 1, .gap = 1, .lookback = 4});
  time_workflow.on({"x1", "x2"}).add(stage::standard_scaler());

  CrossValidationOptions raw_time_options{};
  raw_time_options.plan = CVPlanKind::TSCrossVal;
  const CrossValidationOptions effective_time =
      validate::detail::effective_time_options_(raw_time_options, time_workflow);
  if (effective_time.time_gap != 1 || effective_time.time_test_size != 1 || effective_time.time_max_train_size != 4) {
    if (err) *err << "time validation should merge Workflow time options into effective CV options\n";
    return false;
  }

  Payload no_time_payload = payload::make(frame).features({"x1", "x2", "category"}).label("y").note(DataNotePolicy::Factorize);
  if (!time_payload.has_time() || no_time_payload.has_time()) {
    if (err) *err << "time validation fixtures should distinguish explicit and absent Payload time roles\n";
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
