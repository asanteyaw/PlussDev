#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <map>
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

models::Model empty_model() {
  return models::Model(::pluss::ai::nodej::Workflow{});
}

// Linearly separable two-class data -- any of the three registered models
// should do reasonably well, so benchmark() just needs to run end-to-end and
// pick *some* finite winner, not any particular one.
struct Frame {
  DataFrame x;
  Series y = pluss::series::undefined();
};

Frame make_frame() {
  const Series x1 = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, f64());
  const Series x2 = pluss::make_series(std::vector<double>{1, 1, 2, 2, 8, 8, 9, 9}, {8}, f64());
  const Series y = pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64());
  return Frame{
      .x = DataFrame({
          {"x1", pluss::table::Thread(x1)},
          {"x2", pluss::table::Thread(x2)},
      }),
      .y = y,
  };
}

bool run_builder_construction_checks(std::ostream* err) {
  models::Model ctor_logreg = empty_model();
  models::Model ctor_tree = empty_model();
  models::Builder built = models::build({
      {"ctor_logreg", ctor_logreg},
      {"ctor_tree", ctor_tree},
  });

  if (built.size() != 2 || !built.has("ctor_logreg") || !built.has("ctor_tree")) {
    if (err) *err << "builder::build({{\"name\", model}, ...}) should construct a populated Builder\n";
    return false;
  }

  const std::vector<std::string> names = built.names();
  if (names != std::vector<std::string>{"ctor_logreg", "ctor_tree"}) {
    if (err) *err << "builder::build({{\"name\", model}, ...}) should preserve construction order\n";
    return false;
  }

  models::Model copied_logreg = empty_model();
  models::Model copied_tree = empty_model();
  models::Builder copied = models::build({
      {"linear regression", copied_logreg},
      {"decision_tree_clsfr", copied_tree},
      {"inline logistic", empty_model()},
  });

  if (copied.size() != 3 || !copied.has("linear regression") || !copied.has("decision_tree_clsfr") ||
      !copied.has("inline logistic")) {
    if (err) *err << "builder::build({{\"name\", model}, ...}) should accept lvalues and inline prvalues\n";
    return false;
  }

  std::map<std::string, models::Model> registry;
  registry.emplace("map_logreg", empty_model());
  registry.emplace("map_tree", empty_model());
  models::Builder from_map = models::build(registry);
  if (from_map.size() != 2 || !from_map.has("map_logreg") || !from_map.has("map_tree")) {
    if (err) *err << "builder::build(map) should copy a named model map cheaply\n";
    return false;
  }

  std::vector<std::pair<std::string, models::Model>> list_registry;
  list_registry.emplace_back("vector_logreg", empty_model());
  list_registry.emplace_back("vector_tree", empty_model());
  models::Builder from_vector = models::build(list_registry);
  if (from_vector.size() != 2 || !from_vector.has("vector_logreg") || !from_vector.has("vector_tree")) {
    if (err) *err << "builder::build(vector) should copy a named model vector cheaply\n";
    return false;
  }

  models::Model source = models::logistic_regression({});
  models::Builder cow = models::build({{"source", source}});
  cow.get("source").standard_scaler();
  if (source.stage_count() != 0 || cow.get("source").stage_count() != 1) {
    if (err) *err << "copied Model handles should detach on mutation without changing the source model\n";
    return false;
  }

  return true;
}

bool run_builder_checks(std::ostream* err) {
  models::Builder builder = models::build();

  if (!builder.empty() || builder.size() != 0) {
    if (err) *err << "a freshly-built Builder should be empty\n";
    return false;
  }
  if (builder.benchmarked()) {
    if (err) *err << "a freshly-built Builder should not report benchmarked()\n";
    return false;
  }

  // Prvalues built directly at the call site -- zero std::move needed.
  builder.inject("logreg", models::logistic_regression({.fit_intercept = true}))
         .inject("tree", models::decision_tree_classifier({.max_depth = 3}))
         .inject("knn", models::knn_classifier({.k = 3}));

  if (builder.size() != 3) {
    if (err) *err << "expected 3 registered models, got " << builder.size() << "\n";
    return false;
  }
  if (!builder.has("logreg") || !builder.has("tree") || !builder.has("knn") || builder.has("nope")) {
    if (err) *err << "has() did not report registered/unregistered models correctly\n";
    return false;
  }

  const std::vector<std::string> names = builder.names();
  if (names != std::vector<std::string>{"logreg", "tree", "knn"}) {
    if (err) *err << "names() should preserve insertion order\n";
    return false;
  }
  const std::vector<std::string> preview = builder.list(2);
  if (preview != std::vector<std::string>{"logreg", "tree"}) {
    if (err) *err << "list(2) should return the first 2 names\n";
    return false;
  }

  // An already-named lvalue Model should copy cheaply; neither side should
  // need std::move for ordinary coordinator construction.
  models::Model extra = models::gaussian_nb_classifier({});
  builder.inject("nb", extra);
  if (builder.size() != 4) {
    if (err) *err << "inject(name, lvalue) did not register the model\n";
    return false;
  }

  // Duplicate name must throw, not silently overwrite.
  bool threw_dup = false;
  try {
    builder.inject("logreg", models::logistic_regression({}));
  } catch (const std::exception&) {
    threw_dup = true;
  }
  if (!threw_dup) {
    if (err) *err << "inject() with a duplicate name should throw\n";
    return false;
  }

  const pluss::table::DataFrame described = builder.report();
  if (described.shape().first != 4) {
    if (err) *err << "report() should have one row per registered model, got " << described.shape().first << "\n";
    return false;
  }

  bool threw_unbenchmarked = false;
  try {
    (void)builder.best();
  } catch (const std::exception&) {
    threw_unbenchmarked = true;
  }
  if (!threw_unbenchmarked) {
    if (err) *err << "best() should require a previous benchmark()\n";
    return false;
  }

  // eject() removes and hands back ownership.
  models::Model ejected = builder.eject("nb");
  if (builder.size() != 3 || builder.has("nb")) {
    if (err) *err << "eject() should remove the model from the Builder\n";
    return false;
  }
  (void)ejected;

  // candidates() clones -- the Builder must still hold every model it had.
  const std::vector<selection::Candidate> candidates = builder.candidates();
  if (candidates.size() != 3 || builder.size() != 3) {
    if (err) *err << "candidates() should clone, not consume, the Builder's models\n";
    return false;
  }

  // benchmark() should spot-check all 3 remaining models without the caller
  // building a single Candidate by hand, and leave the Builder untouched.
  // 2 folds -- the default fold count leaves too few of these 8 rows per
  // test fold once split by class.
  selection::CrossValOptions cv{};
  cv.folds = 2;
  cv.shuffle = false;
  const Frame frame = make_frame();
  const selection::CompareResult result = builder.benchmark(frame.x, frame.y, cv, TrainOptions{}, AssessOptions{},
                                                            MetricKind::Accuracy, TuneObjectivePolicy::Auto,
                                                            selection::FitOptions{.fit_top = 2});
  if (result.report.trials.size() != 3) {
    if (err) *err << "benchmark() should have produced one trial per held model, got "
                  << result.report.trials.size() << "\n";
    return false;
  }
  if (!result.best().trained()) {
    if (err) *err << "benchmark()'s best() model should be trained\n";
    return false;
  }
  if (!result.worst().trained()) {
    if (err) *err << "benchmark()'s worst() model should be trained\n";
    return false;
  }
  if (!builder.benchmarked() || !builder.best().trained() || !builder.worst().trained()) {
    if (err) *err << "Builder should remember the trained best() and worst() models from its last benchmark()\n";
    return false;
  }
  const std::vector<std::pair<std::string, models::Model>> top2 = result.top(2);
  if (top2.size() != 2 || top2[0].first.empty() || !top2[0].second.trained() || !top2[1].second.trained()) {
    if (err) *err << "benchmark()'s top(k) should return named, trained Models\n";
    return false;
  }
  if (builder.size() != 3) {
    if (err) *err << "benchmark() should not consume the Builder's own models\n";
    return false;
  }

  // benchmark(LabeledBatch, ...) / selection::compare(candidates, LabeledBatch,
  // ...) -- the direct entry point that skips DataFrame entirely (DataFrame
  // is deliberately CPU-only; LabeledBatch is raw Series, so this is what a
  // caller with GPU-resident Series data reaches for). Built via the same
  // cross_validate_labeled_from_xy_(...) helper the DataFrame-taking
  // overloads already use internally, so this is a real, already-exercised
  // construction path -- not a hand-rolled one -- to check
  // benchmark(LabeledBatch,...) actually runs and produces the same trial
  // count, not just that it compiles.
  const std::vector<std::string> feature_names = cross_validate_default_feature_names_(frame.x, "test");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(frame.x, frame.y, feature_names, "test");
  const selection::CompareResult batch_result = builder.benchmark(batch, cv);
  if (batch_result.report.trials.size() != 3 || !batch_result.best().trained()) {
    if (err) *err << "benchmark(LabeledBatch,...) should behave like benchmark(x,y,...)\n";
    return false;
  }
  const selection::CompareResult direct_result = selection::compare(builder.candidates(), batch, cv);
  if (direct_result.report.trials.size() != 3 || !direct_result.best().trained()) {
    if (err) *err << "selection::compare(candidates, LabeledBatch, ...) should work directly too\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("lymph Builder construction", "[lymph][builder]") {
  if (!run_builder_construction_checks(&std::cerr)) {
    FAIL("lymph Builder construction checks failed");
  }
}

TEST_CASE("lymph Builder", "[lymph][builder]") {
  if (!run_builder_checks(&std::cerr)) {
    FAIL("lymph Builder checks failed");
  }
}
#else
int main() {
  if (!run_builder_construction_checks(&std::cerr)) {
    return 1;
  }
  if (!run_builder_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "lymph Builder checks passed\n";
  return 0;
}
#endif
