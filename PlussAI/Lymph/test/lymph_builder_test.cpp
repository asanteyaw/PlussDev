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

// Linearly separable two-class data -- any of the three registered models
// should do reasonably well, so benchmark() just needs to run end-to-end and
// pick *some* finite winner, not any particular one.
struct Frame {
  DataFrame x;
  Series y;
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

bool run_builder_checks(std::ostream* err) {
  Builder builder = builder::build();

  if (!builder.empty() || builder.size() != 0) {
    if (err) *err << "a freshly-built Builder should be empty\n";
    return false;
  }

  // Prvalues built directly at the call site -- zero std::move needed.
  builder.inject("logreg", model::logistic_regression({.fit_intercept = true}))
         .inject("tree", model::decision_tree_classifier({.max_depth = 3}))
         .inject("knn", model::knn_classifier({.k = 3}));

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

  // An already-named lvalue Model -- std::move is required here (Model has
  // no copy constructor), matching what the redesign plan documents.
  Model extra = model::gaussian_nb_classifier({});
  builder.inject("nb", std::move(extra));
  if (builder.size() != 4) {
    if (err) *err << "inject(name, std::move(lvalue)) did not register the model\n";
    return false;
  }

  // Duplicate name must throw, not silently overwrite.
  bool threw_dup = false;
  try {
    builder.inject("logreg", model::logistic_regression({}));
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

  // eject() removes and hands back ownership.
  Model ejected = builder.eject("nb");
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
  CrossValidationOptions cv{};
  cv.folds = 2;
  cv.shuffle = false;
  const Frame frame = make_frame();
  const selection::CompareResult result = builder.benchmark(frame.x, frame.y, cv);
  if (result.report.trials.size() != 3) {
    if (err) *err << "benchmark() should have produced one trial per held model, got "
                  << result.report.trials.size() << "\n";
    return false;
  }
  if (!result.best().trained()) {
    if (err) *err << "benchmark()'s best() model should be trained\n";
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
TEST_CASE("lymph Builder", "[lymph][builder]") {
  if (!run_builder_checks(&std::cerr)) {
    FAIL("lymph Builder checks failed");
  }
}
#else
int main() {
  if (!run_builder_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "lymph Builder checks passed\n";
  return 0;
}
#endif
