#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <numeric>
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

std::vector<double> as_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

ModelSpec linear_spec() {
  return ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::None,
          .fit_intercept = true,
      },
  };
}

// A raw frame with an "id" column that must never reach the model and a
// couple of NaN feature rows that must not reach it either -- the two things
// Policy exists to strip on every learn_/infer/save call, replayably.
DataFrame make_raw_frame(int n) {
  std::vector<double> id_values;
  std::vector<double> x_values;
  std::vector<double> y_values;
  for (int i = 0; i < n; ++i) {
    id_values.push_back(1000.0 + i);
    const double x = static_cast<double>(i) * 0.4;
    if (i == 2 || i == 5) {
      x_values.push_back(std::numeric_limits<double>::quiet_NaN());
    } else {
      x_values.push_back(x);
    }
    y_values.push_back(3.0 * x + 1.0);
  }
  const Series id_series = pluss::make_series(id_values, {static_cast<std::int64_t>(n)}, f64());
  const Series x_series = pluss::make_series(x_values, {static_cast<std::int64_t>(n)}, f64());
  const Series y_series = pluss::make_series(y_values, {static_cast<std::int64_t>(n)}, f64());
  return DataFrame({
      {"id", Thread(id_series)},
      {"x", Thread(x_series)},
      {"y", Thread(y_series)},
  });
}

DataFrameBatchOptions xy_data_options() {
  DataFrameBatchOptions options{};
  options.feature_names = {"x"};
  options.label_feature = "y";
  return options;
}

bool run_workflow_checks(std::ostream* err) {
  const DataFrame raw = make_raw_frame(12);

  // 1) Policy alone: "id" gone, NaN rows gone, "x"/"y" untouched.
  workflow::Policy cleanup{.drop_columns = {"id"}, .filter_nulls = true, .null_filter_columns = {"x"}};
  const DataFrame cleaned = cleanup.apply(raw);
  if (cleaned.ncases() != 10) {
    if (err) *err << "Policy::apply should drop exactly the 2 NaN-x rows, got " << cleaned.ncases() << " rows\n";
    return false;
  }
  const auto cleaned_names = cleaned.feataxis().series().features();
  for (const auto& name : cleaned_names) {
    if (name == "id") {
      if (err) *err << "Policy::apply should have dropped the \"id\" column\n";
      return false;
    }
  }

  workflow::Policy admission{.drop_columns = {"id"},
                             .require_columns = {"id", "x", "y"},
                             .reject_missing = {"y"},
                             .allow_missing = {"x"}};
  admission.forbid({"status"}).reject({"y"}).allow({"x"});
  const DataFrame admitted = admission.apply(raw);
  if (admitted.ncases() != 12 || admitted.features().size() != 2) {
    if (err) *err << "Policy admission rules should drop id while leaving imputer-owned missing x rows intact\n";
    return false;
  }

  workflow::Policy legacy_guard{.drop_columns = {"id"}, .allow_missing = {"x"}, .filter_nulls = true};
  const DataFrame legacy_guarded = legacy_guard.apply(raw);
  if (legacy_guarded.ncases() != 12) {
    if (err) *err << "Policy allow_missing should exempt imputer-owned feature columns from broad legacy filtering\n";
    return false;
  }

  bool threw_required = false;
  try {
    (void)workflow::Policy{.require_columns = {"missing_raw_column"}}.apply(raw);
  } catch (const pluss::Error&) {
    threw_required = true;
  }
  if (!threw_required) {
    if (err) *err << "Policy should reject frames missing required raw columns\n";
    return false;
  }

  bool threw_forbidden = false;
  try {
    (void)workflow::Policy{.forbid_columns = {"id"}}.apply(raw);
  } catch (const pluss::Error&) {
    threw_forbidden = true;
  }
  if (!threw_forbidden) {
    if (err) *err << "Policy should reject forbidden raw columns unless explicitly dropped\n";
    return false;
  }

  // 2) Workflow: raw frame in (with "id" and NaNs), correct predictions out.
  workflow::Workflow deployed = workflow::make(models::Model(linear_spec()), cleanup);
  deployed.learn_(raw, xy_data_options());
  if (!deployed.trained() || !deployed.has_model_spec() || deployed.task_kind() != TaskKind::Regression) {
    if (err) *err << "Workflow should expose trained/model metadata from its held model\n";
    return false;
  }
  const Series predicted = deployed.infer(raw.drop({"y"}));
  const auto predicted_values = as_f64(predicted);
  if (predicted_values.size() != 10) {
    if (err) *err << "Workflow::infer should return one prediction per surviving row (10), got "
                  << predicted_values.size() << "\n";
    return false;
  }

  const MetricReport report = deployed.assess(raw, xy_data_options());
  const double r2 = report.r2.item<double>();
  if (r2 < 0.999) {
    if (err) *err << "Workflow::assess should score close to R2=1 on noiseless linear data, got " << r2 << "\n";
    return false;
  }

  // 3) Workflow is cheap-copy ergonomic like Model: learn(...) returns a
  // trained copy without forcing std::move or mutating the source object.
  workflow::Workflow source = workflow::make(models::Model(linear_spec()), cleanup);
  const workflow::Workflow learned = source.learn(raw, xy_data_options());
  if (source.trained() || !learned.trained()) {
    if (err) *err << "Workflow::learn(...) should train a copied workflow and leave the source untrained\n";
    return false;
  }
  if (learned.infer(raw.drop({"y"})).numel() != 10) {
    if (err) *err << "Workflow::learn(...) result should infer through the copied trained model\n";
    return false;
  }

  // 3b) Phase-2/6 lean step surface: Workflow exposes a compact routed
  // accessor, but durable transformation breadth comes from Stage objects.
  workflow::Workflow routed = workflow::make(models::Model(linear_spec()), cleanup);
  prep::SimpleImputerOptions median_fill{};
  median_fill.strategy = prep::SimpleImputerStrategy::Median;
  routed.on({"x"}).add(prep::stage::simple_imputer(median_fill)).add(prep::stage::standard_scaler());
  const workflow::Workflow routed_learned = routed.learn(raw, xy_data_options());
  if (!routed_learned.trained() || routed_learned.stage_count() != 2 ||
      routed_learned.infer(raw.drop({"y"})).numel() != 10) {
    if (err) *err << "Workflow::on(...).add(stage).add(stage) should attach routed stages and train through them\n";
    return false;
  }
  const prep::TransformContract scaler_contract = prep::stage::standard_scaler().contract();
  const prep::TransformContract imputer_contract = prep::stage::simple_imputer().contract();
  if (!scaler_contract.reversible || scaler_contract.inverse_available || scaler_contract.lossy ||
      imputer_contract.reversible || !imputer_contract.needs_inverse_memory) {
    if (err) *err << "Stage::contract should expose conservative inverse feasibility metadata\n";
    return false;
  }
  const prep::TransformContract identity_contract = prep::TransformContract{
      .reversible = true,
      .inverse_available = false,
      .lossy = false,
      .stateful = false,
  };
  const prep::CustomStage identity_stage = prep::stage::custom(
      "identity",
      [](const Series& input) -> Series {
        return input;
      },
      identity_contract);
  if (identity_stage.name != "identity" || !identity_stage.map_fn || !identity_stage.contract.reversible ||
      identity_stage.contract.inverse_available || identity_stage.contract.lossy || identity_stage.persistable) {
    if (err) *err << "stage::custom should produce a named, copyable, non-persistable callable stage contract\n";
    return false;
  }
  models::Model custom_model(linear_spec());
  custom_model.add(identity_stage);
  if (!custom_model.has_opaque_transforms()) {
    if (err) *err << "Model::add(custom_stage) should attach callable stages through NodeJ's opaque transform path\n";
    return false;
  }
  workflow::Workflow custom_workflow = workflow::make(models::Model(linear_spec()), cleanup);
  custom_workflow.add(identity_stage);
  if (!custom_workflow.has_opaque_transforms()) {
    if (err) *err << "Workflow::add(custom_stage) should attach whole-feature callable stages\n";
    return false;
  }

  workflow::Workflow prepped = workflow::make(models::Model(linear_spec()), cleanup);
  prepped.prep().scale({"x"});
  const workflow::Workflow prepped_learned = prepped.learn(raw, xy_data_options());
  if (!prepped_learned.trained() || prepped_learned.stage_count() != 1) {
    if (err) *err << "Workflow::prep().scale(...) should expose the short prep verb\n";
    return false;
  }

  // 3c) Phase-3 branch-DAG foundation: a Workflow can be built as a
  // policy/prep design first, then absorb the selected Model later.
  workflow::Workflow design = workflow::make(cleanup);
  design.workers(2);
  design.on("num", {"x"}).add(prep::stage::simple_imputer(median_fill)).add(prep::stage::standard_scaler());
  design.join();
  if (design.branch_count() != 1 || !design.joined() || design.workers() != 2) {
    if (err) *err << "Workflow design should record branch, join, and worker metadata\n";
    return false;
  }
  if (!design.branches()[0].named || design.branches()[0].name != "num" ||
      design.branches()[0].stage_count != 2) {
    if (err) *err << "Workflow named branch should record its name and stage count\n";
    return false;
  }

  const workflow::Workflow candidate = design.with(models::Model(linear_spec()));
  const workflow::Workflow candidate_learned = candidate.learn(raw, xy_data_options());
  if (!candidate_learned.trained() || candidate_learned.stage_count() != 2 ||
      candidate_learned.infer(raw.drop({"y"})).numel() != 10) {
    if (err) *err << "Workflow::with(model) should replay routed design stages onto the selected model\n";
    return false;
  }
  if (design.trained() || candidate.trained()) {
    if (err) *err << "Workflow design and candidate templates should stay untrained after out-of-place learn(...)\n";
    return false;
  }

  workflow::Workflow explicit_branch = workflow::make(cleanup);
  explicit_branch.branch("numeric").route({"x"}).add(prep::stage::standard_scaler());
  if (explicit_branch.branch_count() != 1 || explicit_branch.branches()[0].name != "numeric" ||
      explicit_branch.branches()[0].stage_count != 1) {
    if (err) *err << "Workflow::branch(name).route(cols) should expose explicit branch construction\n";
    return false;
  }

  bool threw_bad_workers = false;
  try {
    (void)workflow::make(cleanup).workers(0);
  } catch (const pluss::Error&) {
    threw_bad_workers = true;
  }
  if (!threw_bad_workers) {
    if (err) *err << "Workflow::workers should reject worker counts below 1\n";
    return false;
  }

  bool threw_empty_branch_name = false;
  try {
    (void)workflow::make(cleanup).on("", {"x"});
  } catch (const pluss::Error&) {
    threw_empty_branch_name = true;
  }
  if (!threw_empty_branch_name) {
    if (err) *err << "Workflow named branch construction should reject empty names\n";
    return false;
  }

  // 3d) Phase-4 typed payload: frame plus label/role metadata travels as
  // one object, while Workflow still uses the frame overload internally so
  // routed stages can resolve column names after Policy cleanup.
  workflow::Payload train_payload = workflow::payload::make(raw).features({"x"}).label("y");
  train_payload.groups(pluss::make_series(std::vector<double>{0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3}, {12}, f64()));
  train_payload.time("event_time");
  if (!train_payload.has_frame() || !train_payload.has_label_feature() ||
      !train_payload.has_groups() || !train_payload.has_time()) {
    if (err) *err << "Payload should retain frame, label, group, and time role metadata\n";
    return false;
  }

  workflow::Workflow payload_workflow = workflow::make(models::Model(linear_spec()), cleanup);
  payload_workflow.on({"x"}).add(prep::stage::simple_imputer(median_fill)).add(prep::stage::standard_scaler());
  payload_workflow.learn_(train_payload);
  if (!payload_workflow.trained() || payload_workflow.stage_count() != 2 ||
      payload_workflow.infer(workflow::payload::make(raw.drop({"y"})).features({"x"})).numel() != 10) {
    if (err) *err << "Workflow should learn and infer through DataFrame-backed Payload objects\n";
    return false;
  }
  if (payload_workflow.assess(train_payload).r2.item<double>() < 0.999) {
    if (err) *err << "Workflow::assess(Payload) should evaluate labeled DataFrame payloads\n";
    return false;
  }

  bool threw_payload_weight_alignment = false;
  try {
    workflow::Payload bad_weight_payload = workflow::payload::make(raw).features({"x"}).label("y").weights(
        pluss::make_series(std::vector<double>{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {12}, f64()));
    workflow::make(models::Model(linear_spec()), cleanup).learn_(bad_weight_payload);
  } catch (const pluss::Error&) {
    threw_payload_weight_alignment = true;
  }
  if (!threw_payload_weight_alignment) {
    if (err) *err << "DataFrame Payload Series weights should fail loudly when Policy can change row alignment\n";
    return false;
  }

  // 4) Batch-level paths stay available for callers already working with
  // Series/FeatureBatch/LabeledBatch. Policy is a raw-frame concern only.
  const LabeledBatch batch = resolve(labeled_batch_from_dataframe(cleaned, xy_data_options()),
                                     "lymph_workflow_test.labeled_batch_from_dataframe");
  workflow::Workflow batch_workflow = workflow::make(models::Model(linear_spec()));
  batch_workflow.learn_(batch);
  if (!batch_workflow.trained() || batch_workflow.infer(batch.features).numel() != batch.features.sample_count()) {
    if (err) *err << "Workflow batch paths should train and infer directly on LabeledBatch/FeatureBatch\n";
    return false;
  }

  // 5) Transform-style models use the same workflow object shape.
  DecomposeFamilyOptions pca{};
  pca.components = 1;
  workflow::Workflow embedder = workflow::make(models::pca(pca), cleanup);
  const Series mapped = embedder.learn_map(raw.drop({"y"}));
  if (!embedder.trained() || mapped.numel() != 10) {
    if (err) *err << "Workflow::learn_map should support transform-style models after Policy cleanup\n";
    return false;
  }

  // 5b) learn_map must resolve stages appended through the branch/route DSL
  // before mapping, exactly like learn_ does -- not silently skip them
  // because they were never folded into the underlying flow yet. map(...)
  // on a workflow with unresolved pending stages must fail loudly instead
  // of silently passing raw features through.
  workflow::Workflow pending = workflow::make(models::Model(linear_spec()), cleanup);
  pending.on({"x"}).add(prep::stage::standard_scaler());
  bool threw_map_pending = false;
  try {
    (void)pending.map(raw.drop({"y"}));
  } catch (const pluss::Error&) {
    threw_map_pending = true;
  }
  if (!threw_map_pending) {
    if (err) *err << "Workflow::map should reject a workflow with unresolved pending stages\n";
    return false;
  }
  const Series learn_mapped = pending.learn_map(raw.drop({"y"}));
  if (pending.stage_count() != 1) {
    if (err) *err << "Workflow::learn_map should resolve pending branch/route stages, stage_count stayed "
                  << pending.stage_count() << "\n";
    return false;
  }
  const auto learn_mapped_values = as_f64(learn_mapped);
  const double mapped_mean =
      std::accumulate(learn_mapped_values.begin(), learn_mapped_values.end(), 0.0) / learn_mapped_values.size();
  if (std::abs(mapped_mean) > 1e-6) {
    if (err) *err << "Workflow::learn_map should standardize through the resolved scaler stage (mean ~0), got "
                  << mapped_mean << "\n";
    return false;
  }

  // 6) Separate x/y overloads cannot safely use a row-dropping Policy,
  // because y would not be filtered with x. That must fail loudly.
  bool threw_row_filter_guard = false;
  try {
    (void)workflow::make(models::Model(linear_spec()), cleanup)
        .learn_(raw.drop({"y"}),
                pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, f64()));
  } catch (const pluss::Error&) {
    threw_row_filter_guard = true;
  }
  if (!threw_row_filter_guard) {
    if (err) *err << "Workflow x/y overloads should reject row-dropping policies\n";
    return false;
  }

  // 7) Save/load round-trip: a fresh Workflow loaded from disk should
  // apply the exact same policy and reproduce the same predictions.
  const std::string path = (std::filesystem::temp_directory_path() / "lymph_workflow_test_model.bin").string();
  deployed.save(path, raw, xy_data_options());

  workflow::Workflow loaded = workflow::load(path);
  if (loaded.policy().drop_columns != cleanup.drop_columns) {
    if (err) *err << "workflow::load should restore drop_columns from the sidecar\n";
    return false;
  }
  if (loaded.policy().null_filter_columns != cleanup.null_filter_columns || !loaded.policy().filter_nulls) {
    if (err) *err << "workflow::load should restore the null-filter policy from the sidecar\n";
    return false;
  }
  const std::string policy_path = (std::filesystem::temp_directory_path() / "lymph_policy_v2_roundtrip.bin").string();
  workflow::save_policy(policy_path, admission);
  const workflow::Policy reloaded_policy = workflow::load_policy(policy_path);
  if (reloaded_policy.drop_columns != admission.drop_columns ||
      reloaded_policy.require_columns != admission.require_columns ||
      reloaded_policy.forbid_columns != admission.forbid_columns ||
      reloaded_policy.reject_missing != admission.reject_missing ||
      reloaded_policy.allow_missing != admission.allow_missing) {
    if (err) *err << "save_policy/load_policy should round-trip the strengthened Policy schema fields\n";
    return false;
  }
  std::filesystem::remove(policy_path + ".policy");

  const Series reloaded_predicted = loaded.infer(raw.drop({"y"}));
  const auto reloaded_values = as_f64(reloaded_predicted);
  if (reloaded_values.size() != predicted_values.size()) {
    if (err) *err << "Reloaded Workflow should predict the same row count as before saving\n";
    return false;
  }
  for (std::size_t i = 0; i < predicted_values.size(); ++i) {
    if (std::abs(reloaded_values[i] - predicted_values[i]) > 1e-6) {
      if (err) *err << "Reloaded Workflow prediction " << i << " diverged from the pre-save prediction\n";
      return false;
    }
  }

  std::filesystem::remove(path);
  std::filesystem::remove(path + ".policy");

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("lymph Workflow / Policy", "[lymph][workflow]") {
  if (!run_workflow_checks(&std::cerr)) {
    FAIL("lymph Workflow / Policy checks failed");
  }
}
#else
int main() {
  if (!run_workflow_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "lymph Workflow / Policy checks passed\n";
  return 0;
}
#endif
