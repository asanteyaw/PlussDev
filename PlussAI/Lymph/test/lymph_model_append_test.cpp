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

  const ColumnRouter categorical_router = columnrouter::names({"category"});
  const ColumnRouter numeric_router = columnrouter::remaining_of(categorical_router);

  // 1) append() before any frame exists, resolution deferred to learn_().
  Model model = models::logistic_regression({.regularization_strength = 0.01, .fit_intercept = true});
  model.append(stage::standard_scaler(), numeric_router)
       .append(stage::one_hot_encoder({.max_categories = 8, .include_unknown_bucket = true}), categorical_router);

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

  // 2) A bare LabeledBatch has no column names -- append()'d-but-unresolved
  // stages must throw a clear error rather than silently skip routing.
  bool threw = false;
  try {
    Model unresolved = models::logistic_regression({.fit_intercept = true});
    unresolved.append(stage::standard_scaler(), numeric_router);
    const LabeledBatch bare_batch{};  // never mind its contents -- should throw before touching them
    unresolved.learn_(bare_batch);
  } catch (const std::exception&) {
    threw = true;
  }
  if (!threw) {
    if (err) *err << "learn_(LabeledBatch) with pending append()'d stages should throw, not silently skip them\n";
    return false;
  }

  // 3) The non-mutating learn(...) sibling clones first; pending stages must
  // travel with the clone and still resolve correctly there.
  Model unlearned = models::logistic_regression({.regularization_strength = 0.01, .fit_intercept = true});
  unlearned.append(stage::standard_scaler(), numeric_router)
           .append(stage::one_hot_encoder({.max_categories = 8, .include_unknown_bucket = true}), categorical_router);
  const Model learned = unlearned.learn(frame, data_options);
  if (unlearned.trained()) {
    if (err) *err << "learn(...) (non-mutating) should leave the original Model untouched\n";
    return false;
  }
  if (!learned.trained() || learned.stage_count() != 2) {
    if (err) *err << "learn(...) (non-mutating) should return a trained clone with both pending stages materialized\n";
    return false;
  }

  // 4) Object-factory preparation accessor: once the Model exists, users
  // should be able to keep preprocessing object/method based instead of
  // stitching stage:: and columnrouter:: factories through every line.
  Model fluent = models::logistic_regression({.regularization_strength = 0.01, .fit_intercept = true});
  auto prep = fluent.prepare();
  const ColumnRouter categorical = prep.names({"category"});
  prep.standardize(prep.remaining(categorical))
      .encode(categorical, {.max_categories = 8, .include_unknown_bucket = true});
  if (fluent.stage_count() != 0) {
    if (err) *err << "prepare().* stages should stay pending until a frame-backed learn_() resolves them\n";
    return false;
  }
  const Model fluent_learned = fluent.learn(frame, data_options);
  if (!fluent_learned.trained() || fluent_learned.stage_count() != 2) {
    if (err) *err << "prepare().standardize(...).encode(...) should materialize into two trained stages\n";
    return false;
  }

  Model shaped = models::logistic_regression({.regularization_strength = 0.01, .fit_intercept = true});
  auto shape = shaped.prepare();
  shape.gaussianize({"x1"}).power(shape.names({"x2"}));
  const Model shaped_learned = shaped.learn(frame, data_options);
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
