#include <pluss/ai/ml.h>
#include <pluss/mic.h>

#include <cmath>
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
using namespace pluss::ai::nodej;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

std::vector<double> as_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

DataFrame make_regression_frame() {
  const Series x0 = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0, 4.0, 5.0}, {6}, f64());
  const Series x1 = pluss::make_series(std::vector<double>{1.0, 0.0, 2.0, 1.0, 3.0, 2.0}, {6}, f64());
  const Series y = x0.mul(2.0).sub(x1.mul(0.5)).add(1.0);
  const Series w = pluss::make_series(std::vector<double>{1.0, 1.0, 0.8, 1.2, 1.0, 0.9}, {6}, f64());

  return DataFrame({
      {"x0", Thread(x0)},
      {"x1", Thread(x1)},
      {"target", Thread(y)},
      {"weight", Thread(w)},
  });
}

DataFrame make_mixed_frame() {
  const Series x0 = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0, 4.0, 5.0}, {6}, f64());
  const auto note = pluss::make_note({"red", "blue", "red", "green", "blue", "red"});
  const Series epoch_nanos = pluss::make_series(
      std::vector<std::int64_t>{
          1'700'000'000'000'000'000LL, 1'700'000'001'000'000'000LL, 1'700'000'002'000'000'000LL,
          1'700'000'003'000'000'000LL, 1'700'000'004'000'000'000LL, 1'700'000'005'000'000'000LL},
      {6},
      Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU));
  const Thread ts = Thread(epoch_nanos).cast().timeline();
  const Series y = x0.mul(0.75).add(2.0);

  return DataFrame({
      {"x0", Thread(x0)},
      {"color", Thread(note)},
      {"ts", ts},
      {"target", Thread(y)},
  });
}

bool run_nodej_dataframe_bridge_checks(std::ostream* err = nullptr) {
  const DataFrame frame = make_regression_frame();

  DataFrameBatchOptions options{};
  options.feature_names = {"x0", "x1", "target", "weight"};
  options.label_feature = "target";
  options.sample_weight_feature = "weight";
  options.layout = ObservationLayout::Rows;

  auto labeled_or = labeled_batch_from_dataframe(frame, options);
  if (!labeled_or.ok()) {
    if (err) *err << "labeled_batch_from_dataframe should succeed for numeric frame\n";
    return false;
  }

  const LabeledBatch& batch = labeled_or.value();
  if (batch.features.sample_count() != 6 || batch.features.feature_count() != 2) {
    if (err) *err << "adapter should keep only requested feature columns (excluding label/weight)\n";
    return false;
  }
  if (!batch.sample_weights.defined() || batch.sample_weights.numel() != 6) {
    if (err) *err << "adapter should map optional sample weights\n";
    return false;
  }

  ModelSpec spec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-8,
          .fit_intercept = true,
      },
  };

  auto reg_or = make_regressor(spec);
  if (!reg_or.ok()) {
    if (err) *err << "make_regressor should succeed for linear regression\n";
    return false;
  }

  auto reg = reg_or.move_value();
  if (!reg->learn(batch).ok()) {
    if (err) *err << "regressor learn should succeed with DataFrame-adapted batch\n";
    return false;
  }

  auto metrics_or = reg->assess(batch, AssessOptions{.metrics = {MetricKind::Mse, MetricKind::R2}});
  if (!metrics_or.ok()) {
    if (err) *err << "regressor assess should succeed with DataFrame-adapted batch\n";
    return false;
  }

  const auto metrics = as_f64(metrics_or.value());
  if (metrics.size() != 2 || metrics[0] > 1e-9 || metrics[1] < 0.999999) {
    if (err) *err << "DataFrame-adapted linear regression should recover exact synthetic signal\n";
    return false;
  }

  DataFrameBatchOptions bad_options{};
  bad_options.label_feature = "missing_label";
  auto bad_or = labeled_batch_from_dataframe(frame, bad_options);
  if (bad_or.ok() || bad_or.status().code != StatusCode::InvalidArgument) {
    if (err) *err << "adapter should reject unknown label feature names\n";
    return false;
  }

  const DataFrame mixed = make_mixed_frame();
  DataFrameBatchOptions mixed_options{};
  mixed_options.feature_names = {"x0", "color", "ts", "target"};
  mixed_options.label_feature = "target";
  mixed_options.thread_policy = DataThreadPolicy::CastTemporalToSeries;
  mixed_options.note_policy = DataNotePolicy::Hash;
  auto mixed_or = labeled_batch_from_dataframe(mixed, mixed_options);
  if (!mixed_or.ok()) {
    if (err) *err << "mixed-schema adapter should hash Note columns and cast temporal columns\n";
    return false;
  }
  if (mixed_or.value().features.feature_count() != 3 || mixed_or.value().features.sample_count() != 6) {
    if (err) *err << "mixed-schema adapter should emit three routed features\n";
    return false;
  }
  if (!mixed_or.value().features.x.isfinite().all().item<bool>()) {
    if (err) *err << "mixed-schema adapter should emit finite hashed/casted features\n";
    return false;
  }

  DataFrameBatchOptions drop_note = mixed_options;
  drop_note.note_policy = DataNotePolicy::Drop;
  auto drop_or = labeled_batch_from_dataframe(mixed, drop_note);
  if (!drop_or.ok() || drop_or.value().features.feature_count() != 2) {
    if (err) *err << "note drop policy should remove Note-backed predictors\n";
    return false;
  }

  DataFrameBatchOptions reject_note = mixed_options;
  reject_note.note_policy = DataNotePolicy::Reject;
  auto reject_or = labeled_batch_from_dataframe(mixed, reject_note);
  if (reject_or.ok() || reject_or.status().code != StatusCode::InvalidArgument) {
    if (err) *err << "note reject policy should fail on Note-backed predictors\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej dataframe adapters from PlussMic") {
  REQUIRE(run_nodej_dataframe_bridge_checks(&std::cerr));
}
#else
int main() {
  if (!run_nodej_dataframe_bridge_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej_dataframe_bridge_test passed\n";
  return 0;
}
#endif
