#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

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
using namespace pluss::ai::lymph;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

struct Frame {
  DataFrame x;
  Series y;
};

Frame make_frame() {
  const Series x1 = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, f64());
  const Series x2 = pluss::make_series(std::vector<double>{1000, 2000, 500, 4000, 1500, 3000, 800, 3500}, {8}, f64());
  const Series y = pluss::make_series(std::vector<double>{0, 0, 0, 1, 0, 1, 0, 1}, {8}, f64());
  return Frame{
      .x = DataFrame({
          {"x1", pluss::table::Thread(x1)},
          {"x2", pluss::table::Thread(x2)},
      }),
      .y = y,
  };
}

// Two numeric predictors on wildly different scales, noiseless linear-in-
// the-standardized-features target: only separable if the make_model
// closure's attached standard_scaler actually ran -- i.e. only passes if
// Search::run genuinely retrained a full Model (prep included) per trial,
// not a bare ModelSpec the way the old GridAxis-based spec search does.
LabeledBatch make_batch(const Frame& frame) {
  const auto names = cross_validate_default_feature_names_(frame.x, "test");
  return cross_validate_labeled_from_xy_(frame.x, frame.y, names, "test");
}

bool run_search_checks(std::ostream* err) {
  const Frame frame = make_frame();
  const LabeledBatch batch = make_batch(frame);

  const std::vector<GridAxis> axes = {
      {.name = "regularization_strength", .values = {0.01, 1.0}},
  };
  const auto make_model = [](const GridPoint& point) {
    Model m = models::logistic_regression({
        .regularization_strength = point.at("regularization_strength"),
        .fit_intercept = true,
    });
    m.standard_scaler();
    return m;
  };

  CrossValidationOptions cv{};
  cv.plan = CVPlanKind::CrossVal;
  cv.folds = 2;
  cv.shuffle = false;

  TrainOptions train_options{};
  train_options.max_iterations = 120;
  train_options.tolerance = 1e-5;

  Search search = search::grid(axes, cv);
  search::Result tuned = search.run(make_model, batch, train_options, AssessOptions{},
                                  MetricKind::Accuracy, TuneObjectivePolicy::Maximize,
                                  selection::FitOptions{.fit_top = 2});

  if (tuned.report.trials.size() != 2) {
    if (err) *err << "Search::run(grid) should have produced one trial per grid point, got "
                  << tuned.report.trials.size() << "\n";
    return false;
  }
  if (!tuned.best().trained()) {
    if (err) *err << "Search::run should return a trained best() model\n";
    return false;
  }
  if (tuned.best().stage_count() != 1) {
    if (err) *err << "Search::run's best() model should carry the attached standard_scaler stage, got stage_count()="
                  << tuned.best().stage_count() << "\n";
    return false;
  }
  const Series predicted = tuned.best().infer(batch.features);
  if (predicted.numel() != 8) {
    if (err) *err << "Search::run's best() model should infer one prediction per row\n";
    return false;
  }

  const std::vector<std::pair<std::string, Model>> top2 = tuned.top(2);
  if (top2.size() != 2 || !top2[0].second.trained() || !top2[1].second.trained()) {
    if (err) *err << "Search::run's top(k) should return k trained, owned Models\n";
    return false;
  }
  if (top2[0].first.find("regularization_strength=") == std::string::npos) {
    if (err) *err << "Search::run's top(k) names should be the winning GridPoint's own label()\n";
    return false;
  }

  // search::build(axes, TuneOptions) bridges the old axis/options pair into
  // the object surface without adding a second random-search training pass to
  // this already-heavy test.
  const Search random_cv_search = search::random(axes, /*trials=*/2, cv);
  (void)random_cv_search;

  TuneOptions tune_options{};
  tune_options.search = SearchKind::Random;
  tune_options.trials = 1;
  tune_options.split = make_cross_validation_split_options(cv);
  tune_options.max_parallel = 2;
  tune_options.random = RandomState{.seed = 7, .deterministic = true};
  Search random_search = search::build(axes, tune_options);
  search::Result random_tuned = random_search.run(make_model, batch, train_options);
  if (random_tuned.report.trials.size() != 1 || !random_tuned.best().trained()) {
    if (err) *err << "search::build(axes, TuneOptions) should preserve random trial count and stored split policy\n";
    return false;
  }

  const DataFrame raw({
      {"x1", pluss::table::Thread(frame.x.get("x1").value())},
      {"x2", pluss::table::Thread(frame.x.get("x2").value())},
      {"y", pluss::table::Thread(frame.y)},
  });
  const Payload raw_payload = payload::make(raw).features({"x1", "x2"}).label("y");
  const Workflow design = workflow::make(Policy{});
  search::Result workflow_tuned = search.run(make_model,
                                           design,
                                           raw_payload,
                                           train_options,
                                           AssessOptions{},
                                           MetricKind::Accuracy,
                                           TuneObjectivePolicy::Maximize,
                                           selection::FitOptions{.fit_top = 1});
  if (workflow_tuned.report.trials.size() != 2 || !workflow_tuned.best().trained()) {
    if (err) *err << "Search::run should consume Workflow + Payload and keep returning trained Models\n";
    return false;
  }
  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("lymph Search", "[lymph][search]") {
  if (!run_search_checks(&std::cerr)) {
    FAIL("lymph Search checks failed");
  }
}
#else
int main() {
  if (!run_search_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "lymph Search checks passed\n";
  return 0;
}
#endif
