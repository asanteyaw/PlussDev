#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

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

LabeledBatch batch() {
  return LabeledBatch{
      .features = FeatureBatch{
          .x = pluss::make_series(std::vector<double>{
                                      1.0, 1.0,
                                      2.0, 1.0,
                                      3.0, 2.0,
                                      4.0, 2.0,
                                      7.0, 8.0,
                                      8.0, 8.0,
                                      9.0, 9.0,
                                      10.0, 9.0,
                                  },
                                  {8, 2},
                                  f64()),
          .layout = ObservationLayout::Rows,
      },
      .y = pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64()),
  };
}

DataFrame frame() {
  return DataFrame({
      {"timestamp", Thread(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0}, {8}, f64()))},
      {"x0", Thread(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 7.0, 8.0, 9.0, 10.0}, {8}, f64()))},
      {"x1", Thread(pluss::make_series(std::vector<double>{1.0, 1.0, 2.0, 2.0, 8.0, 8.0, 9.0, 9.0}, {8}, f64()))},
      {"y", Thread(pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64()))},
  });
}

bool run_spotcheck_checks(std::ostream* err) {
  Builder registry = builder::build({
      {"bayes", model::gaussian_nb_classifier()},
      {"tree", model::decision_tree_classifier({.max_depth = 2})},
  });

  metrics::ValidationOptions cv{};
  cv.folds = 2;
  cv.shuffle = false;
  cv.max_parallel = 1;

  const spotcheck::Result ranked = spotcheck::suite(registry)
                                     .strategy("plain", TrainOptions{})
                                     .run(batch(),
                                          cv,
                                          AssessOptions{},
                                          MetricKind::Accuracy,
                                          TuneObjectivePolicy::Maximize,
                                          selection::FitOptions{.fit_top = 1, .fit_bottom = 2});

  if (ranked.report.trials.size() != 2 || !ranked.best().trained() || !ranked.worst().trained()) {
    if (err) *err << "spotcheck should benchmark every model and expose trained best/worst models\n";
    return false;
  }

  const auto top1 = ranked.top(1);
  const auto bottom2 = ranked.bottom(2);
  if (top1.size() != 1 || bottom2.size() != 2
      || top1.front().first.empty()
      || bottom2.front().first.empty()
      || !top1.front().second.trained()
      || !bottom2.front().second.trained()
      || !bottom2.back().second.trained()) {
    if (err) *err << "spotcheck result should expose trained top(k) and bottom(k) selections\n";
    return false;
  }

  const Payload raw_payload = payload::make(frame()).features({"x0", "x1"}).label("y");
  const Workflow design = workflow::make(Policy{});
  const spotcheck::Result workflow_ranked = spotcheck::suite(registry)
                                              .strategy("plain", TrainOptions{})
                                              .run(design,
                                                   raw_payload,
                                                   cv,
                                                   AssessOptions{},
                                                   MetricKind::Accuracy,
                                                   TuneObjectivePolicy::Maximize,
                                                   selection::FitOptions{.fit_top = 1});
  if (workflow_ranked.report.trials.size() != 2 || !workflow_ranked.best().trained()) {
    if (err) *err << "spotcheck should consume Workflow + Payload without changing the ranked result shape\n";
    return false;
  }

  metrics::ValidationOptions time_cv{};
  time_cv.plan = CVPlanKind::TSCrossVal;
  time_cv.folds = 2;
  time_cv.shuffle = false;
  time_cv.max_parallel = 1;

  Payload time_payload = payload::make(frame())
      .features({"x0", "x1"})
      .label("y")
      .time(TimeOptions{.index = "timestamp", .horizon = 1, .gap = 1, .lookback = 4});
  Workflow time_design = workflow::make(Policy{});
  time_design.time(TimeOptions{.index = "timestamp", .horizon = 1, .gap = 1, .lookback = 4});

  const metrics::ValidationOptions effective_time =
      validate::detail::effective_time_options_(time_cv, time_design);
  if (effective_time.time_gap != 1 || effective_time.time_test_size != 1 || effective_time.time_max_train_size != 4) {
    if (err) *err << "spotcheck should use the shared Workflow/Payload time option merge\n";
    return false;
  }

  if (!time_payload.has_time() || raw_payload.has_time()) {
    if (err) *err << "spotcheck time regression should distinguish payloads with and without explicit time roles\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("lymph spotcheck top and bottom selections", "[lymph][spotcheck]") {
  REQUIRE(run_spotcheck_checks(&std::cerr));
}
#else
int main() {
  if (!run_spotcheck_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "lymph spotcheck checks passed\n";
  return 0;
}
#endif
