#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <cstdint>
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
using pluss::table::Thread;
using namespace pluss::ai::lymph;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

// Two numeric predictors on wildly different scales, one categorical
// predictor, noiseless linear-in-the-standardized-features target: only
// separable if the attached standard_scaler/one_hot_encoder actually ran.
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

bool run_model_append_checks(std::ostream* err) {
  const DataFrame frame = make_frame();
  const DataFrameBatchOptions data_options{.label_feature = "y", .note_policy = DataNotePolicy::Factorize};

  models::Model explicit_linear = models::linear::linear_regression();
  models::Model shortcut_linear = models::linear_regression();
  models::Model plural_tree = models::decision_tree_regressor();
  models::Model grouped_tree = models::tree::decision_tree_regressor();
  models::Model grouped_forest = models::ensemble::random_forest_classifier();
  if (explicit_linear.task_kind() != TaskKind::Regression ||
      shortcut_linear.task_kind() != TaskKind::Regression ||
      plural_tree.task_kind() != TaskKind::Regression ||
      grouped_tree.task_kind() != TaskKind::Regression ||
      grouped_forest.task_kind() != TaskKind::Classification) {
    if (err) *err << "model factories should work through grouped, shortcut, and compatibility namespaces\n";
    return false;
  }

  const prep::Route categorical_router = prep::route::names({"category"});
  const prep::Route numeric_router = prep::route::remaining_of(categorical_router);

  // 1) append() before any frame exists, resolution deferred to learn_().
  models::Model model = models::decision_tree_classifier({.max_depth = 3});
  model.append(prep::stage::standard_scaler(), numeric_router)
       .append(prep::stage::one_hot_encoder({.max_categories = 8, .include_unknown_bucket = true}), categorical_router);

  if (model.stage_count() != 0) {
    if (err) *err << "append()'d stages should not materialize into the Flow until learn_() sees a frame\n";
    return false;
  }

  model.learn_(frame, data_options);

  if (model.stage_count() != 2) {
    if (err) *err << "learn_(frame, data_options) should have materialized both pending stages, got stage_count()="
                  << model.stage_count() << "\n";
    return false;
  }
  if (!model.trained()) {
    if (err) *err << "model should be trained after learn_()\n";
    return false;
  }

  const Series predicted = model.infer(frame.drop({"y"}), data_options);
  if (predicted.numel() != 8) {
    if (err) *err << "infer() should return one prediction per row\n";
    return false;
  }

  // 2) The non-mutating learn(...) sibling clones first; pending stages must
  // travel with the clone and still resolve correctly there.
  models::Model unlearned = models::decision_tree_classifier({.max_depth = 3});
  unlearned.append(prep::stage::standard_scaler(), numeric_router)
           .append(prep::stage::one_hot_encoder({.max_categories = 8, .include_unknown_bucket = true}), categorical_router);
  const models::Model learned = unlearned.learn(frame, data_options);
  if (unlearned.trained()) {
    if (err) *err << "learn(...) (non-mutating) should leave the original Model untouched\n";
    return false;
  }
  if (!learned.trained() || learned.stage_count() != 2) {
    if (err) *err << "learn(...) (non-mutating) should return a trained clone with both pending stages materialized\n";
    return false;
  }

  // 3) Object-factory preparation accessor: once the Model exists, users
  // should be able to keep preprocessing object/method based instead of
  // stitching prep::stage:: and prep::route:: factories through every line.
  models::Model fluent = models::decision_tree_classifier({.max_depth = 3});
  auto prep = fluent.prepare();
  const prep::Route categorical = prep.names({"category"});
  prep.standardize(prep.remaining(categorical))
      .encode(categorical, {.max_categories = 8, .include_unknown_bucket = true});
  if (fluent.stage_count() != 0) {
    if (err) *err << "prepare().* stages should stay pending until a frame-backed learn_() resolves them\n";
    return false;
  }
  const models::Model fluent_learned = fluent.learn(frame, data_options);
  if (!fluent_learned.trained() || fluent_learned.stage_count() != 2) {
    if (err) *err << "prepare().standardize(...).encode(...) should materialize into two trained stages\n";
    return false;
  }

  models::Model shaped = models::decision_tree_classifier({.max_depth = 3});
  auto shape = shaped.prepare();
  shape.gaussianize({"x1"}).power(shape.names({"x2"}));
  const models::Model shaped_learned = shaped.learn(frame, data_options);
  if (!shaped_learned.trained() || shaped_learned.stage_count() != 2) {
    if (err) *err << "prepare().gaussianize(...).power(...) should expose distribution-shaping stages\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("lymph Model::append / Stage", "[lymph][model][append]") {
  if (!run_model_append_checks(&std::cerr)) {
    FAIL("lymph Model::append checks failed");
  }
}
#else
int main() {
  if (!run_model_append_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "lymph Model::append checks passed\n";
  return 0;
}
#endif
