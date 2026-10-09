#include <pluss/ai/procedure.h>
#include <pluss/ai/ice.h>
#include <pluss/ai/lymph.h>
#include <pluss/mic.h>
#include <pluss/pluss.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

namespace ai = pluss::ai;
namespace ice = pluss::ai::ice;
namespace lymph = pluss::ai::lymph;

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::table::DataFrame;
using pluss::table::Slate;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

Series vec(std::vector<double> values) {
  return pluss::make_series(std::move(values), {static_cast<std::int64_t>(values.size())}, f64());
}

DataFrame raw_frame() {
  return DataFrame({
      {"id", Slate(vec({100, 101, 102}))},
      {"x", Slate(vec({1.0, 2.0, 3.0}))},
      {"y", Slate(vec({4.0, 5.0, 6.0}))},
  });
}

lymph::LabeledBatch regression_batch() {
  const Series x = pluss::make_series(std::vector<double>{
                                          0.0, 0.0,
                                          1.0, 0.0,
                                          2.0, 1.0,
                                          3.0, 1.0,
                                          4.0, 2.0,
                                          5.0, 2.0},
                                      {6, 2}, f64());
  const Series y = pluss::make_series(std::vector<double>{0.0, 1.0, 3.0, 4.0, 6.0, 7.0}, {6}, f64());
  return lymph::LabeledBatch{
      .features = lymph::FeatureBatch{.x = x, .layout = lymph::ObservationLayout::Rows},
      .y = y,
  };
}

lymph::TrainOptions rich_train_options() {
  lymph::TrainOptions options{};
  options.layout = lymph::ObservationLayout::Columns;
  options.missing = lymph::MissingPolicy::Drop;
  options.unknown_categorical = lymph::UnknownCategoryPolicy::Ignore;
  options.max_iterations = 77;
  options.tolerance = 1.23e-4;
  options.warm_start = true;
  options.track_diagnostics = false;
  options.random.seed = 42;
  options.random.deterministic = false;
  options.sample_weights = vec({1.0, 2.0, 3.0});
  options.class_weight = lymph::ClassWeightMode::Custom;
  options.class_weights = vec({0.25, 0.75});
  options.calibration = lymph::ScoreCalibration::Isotonic;
  options.calibration_iterations = 55;
  options.calibration_tolerance = 7.7e-4;
  options.parameter_lower_bounds = vec({-1.0, -2.0});
  options.parameter_upper_bounds = vec({3.0, 4.0});
  options.balance.kind = lymph::BalanceTargetKind::Counts;
  options.balance.ratio = 1.25;
  options.balance.counts = vec({5.0, 7.0});
  options.balance.growth = lymph::BalanceGrowthKind::BlendNeighbors;
  options.balance.neighbors = 9;
  options.balance.clean = lymph::BalanceCleanKind::NeighborAgreement;
  options.balance.clean_neighbors = 4;
  options.balance.clean_vote = lymph::BalanceCleanVote::MajorityDisagreement;
  options.balance.protect_small_classes = false;
  return options;
}

lymph::InferOptions rich_infer_options() {
  lymph::InferOptions options{};
  options.layout = lymph::ObservationLayout::Columns;
  options.missing = lymph::MissingPolicy::KeepNaN;
  options.unknown_categorical = lymph::UnknownCategoryPolicy::MissingBucket;
  options.positive_threshold = 0.73;
  return options;
}

lymph::AssessOptions rich_assess_options() {
  lymph::AssessOptions options{};
  options.layout = lymph::ObservationLayout::Columns;
  options.metrics = {lymph::MetricKind::Mae, lymph::MetricKind::R2, lymph::MetricKind::QuantileLoss};
  options.sample_weights = vec({1.0, 0.5, 2.0});
  options.positive_threshold = 0.61;
  options.regression_quantile = 0.27;
  return options;
}

ice::TrainOptions rich_ice_train_options() {
  ice::TrainOptions options{};
  options.batch_size = 3;
  options.learning_rate = 0.02;
  options.optimizer = ice::OptimizerKind::RMSProp;
  options.loss = ice::LossKind::MSE;
  options.shuffle = false;
  options.seed = 123;
  options.standardize = true;
  options.validation.split_ratio = 0.0;
  options.validation.every_n_epochs = 2;
  options.validation.start_from_epoch = 1;
  options.validation.monitor = "val_metric";
  options.validation.direction = ice::MonitorDirection::Max;
  options.architecture_regularization.dropout = 0.15;
  options.objective_regularization.weight_decay = 0.02;
  options.objective_regularization.decoupled_weight_decay = true;
  options.objective_regularization.l1_penalty = 0.01;
  options.objective_regularization.l2_penalty = 0.03;
  options.objective_regularization.label_smoothing = 0.0;
  options.loop_regularization.early_stopping = false;
  options.loop_regularization.patience = 5;
  options.loop_regularization.min_delta = 1.0e-5;
  options.loop_regularization.restore_best_weights = true;
  options.execution.grad_clip.mode = pluss::nn::optim::GradientClipMode::ByValue;
  options.execution.grad_clip.threshold = 0.75;
  options.execution.max_epochs = 4;
  options.execution.accumulation_steps = 2;
  options.execution.fast_dev_batches = 0;
  options.execution.max_train_batches = 0;
  options.execution.max_valid_batches = 0;
  options.execution.fail_on_non_finite = true;
  options.execution.deterministic = true;
  options.execution.precision = ScalarType::Float64;
  options.execution.platform = ice::TrainPlatform::CPU;
  return options;
}

lymph::LabeledBatch tiny_ice_batch() {
  const Series x = pluss::make_series(std::vector<double>{
                                          0.0,
                                          1.0,
                                          2.0,
                                          3.0},
                                      {4, 1}, f64());
  const Series y = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0}, {4, 1}, f64());
  return lymph::LabeledBatch{
      .features = lymph::FeatureBatch{.x = x, .layout = lymph::ObservationLayout::Rows},
      .y = y,
  };
}

std::filesystem::path fresh_bundle_path(const std::string& stem) {
  const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path() / (stem + "_" + std::to_string(tick));
}

ai::Step scale_step(double factor) {
  static bool bound = false;
  if (!bound) {
    ai::procedure::bind(
        ai::procedure::CustomContract{
            .key = "custom::scale",
            .persist_schema_version = 1,
            .component_schema_version = 1,
            .load = [](std::string name,
                       ai::StepOptions options,
                       std::string_view path,
                       const std::vector<std::pair<std::string, std::string>>&) -> ai::Step {
              std::ifstream in{std::string(path)};
              if (!in) {
                ::pluss::throw_error("procedure_phase6 scale_step loader: missing factor artifact.");
              }
              double loaded = 1.0;
              in >> loaded;
              return scale_step(loaded);
            },
        });
    bound = true;
  }

  auto shared_factor = std::make_shared<double>(factor);
  ai::StepOptions options{};
  options.capabilities = {ai::StepCapability::Map};
  options.inputs = {"x"};
  options.outputs = {"y"};
  options.persistable = true;
  options.component = "custom::scale";

  ai::Step::Runner runner =
      [shared_factor](const std::vector<ai::Artifact>& inputs, const ai::RunOptions&) -> ai::Artifact {
        if (inputs.size() != 1 || !inputs.front().is<Series>()) {
          ::pluss::throw_error("procedure_phase6 scale_step expected one Series input.");
        }
        return ai::Artifact("y", ai::Artifact::Value(inputs.front().get<Series>().mul(*shared_factor)));
      };

  return ai::procedure::step(
      "scale",
      std::move(options),
      ai::procedure::CustomPersist{
          .key = "custom::scale",
          .save = [shared_factor](std::string_view path) {
            std::ofstream out(std::string(path), std::ios::trunc);
            if (!out) {
              ::pluss::throw_error("procedure_phase6 scale_step saver: could not open artifact.");
            }
            out << *shared_factor << "\n";
          },
      },
      std::move(runner));
}

ai::Step ice_outline_step() {
  static bool bound = false;
  if (!bound) {
    ai::procedure::bind_ice("ice::outline::phase6", [] {
      return ice::Outline(pluss::nn::Fusion(1, 1));
    });
    bound = true;
  }

  auto model = ice::Outline(pluss::nn::Fusion(1, 1));
  ice::TrainOptions fit{};
  fit.loss = ice::LossKind::MSE;
  fit.execution.max_epochs = 2;
  fit.validation.split_ratio = 0.0;
  fit.batch_size = 4;
  fit.learning_rate = 0.01;
  model.learn(tiny_ice_batch().features.x, tiny_ice_batch().y, fit);
  return ai::procedure::step(
      "ice_model",
      "train",
      "ice_out",
      std::move(model),
      "ice::outline::phase6",
      ice::LossKind::MSE,
      rich_ice_train_options());
}

std::vector<double> as_vec(const Series& values) {
  return values.to(pluss::pCPU, ScalarType::Float64).reshape({values.numel()}).to_vec<double>();
}

bool equal_series_file(const std::filesystem::path& lhs, const std::filesystem::path& rhs) {
  const bool lhs_exists = std::filesystem::exists(lhs);
  const bool rhs_exists = std::filesystem::exists(rhs);
  if (lhs_exists != rhs_exists) return false;
  if (!lhs_exists) return true;
  return as_vec(pluss::io::load<Series>(lhs.string())) == as_vec(pluss::io::load<Series>(rhs.string()));
}

bool run_phase6_checks(std::ostream* err = nullptr) {
  {
    const auto bundle = fresh_bundle_path("procedure_phase6_policy");
    const ai::Procedure proc = ai::procedure::make({
        ai::procedure::step(
            "gate",
            "raw",
            "clean",
            lymph::workflow::Policy{.drop_columns = {"id"}}),
        ai::procedure::step("out", ai::procedure::sink("phase6.clean")),
    }, {
        ai::procedure::link("gate", "out"),
    });
    proc.save(bundle.string());

    const ai::Procedure restored = ai::procedure::load(bundle.string());
    const ai::ProcedureResult out = restored.run({ai::procedure::input("raw", raw_frame())});
    const DataFrame clean = out.get<DataFrame>("clean");
    if (clean.get("id").has_value() || !clean.get("x").has_value() || !clean.get("y").has_value()) {
      if (err) *err << "Procedure phase 6 should reload a persisted Policy step and preserve its admission behavior\n";
      std::filesystem::remove_all(bundle);
      return false;
    }
    const DataFrame sink_out = out.get<DataFrame>("out");
    if (sink_out.get("id").has_value()) {
      if (err) *err << "Procedure phase 6 sink reload should preserve downstream flow under the sink step name\n";
      std::filesystem::remove_all(bundle);
      return false;
    }
    std::filesystem::remove_all(bundle);
  }

  {
    const auto bundle = fresh_bundle_path("procedure_phase6_custom");
    const ai::Procedure proc = ai::procedure::make({scale_step(3.0)});
    proc.save(bundle.string());

    const ai::Procedure restored = ai::procedure::load(bundle.string());
    const ai::ProcedureResult out = restored.run({ai::procedure::input("x", vec({2.0, 4.0, 6.0}))});
    if (as_vec(out.get<Series>("y")) != std::vector<double>{6.0, 12.0, 18.0}) {
      if (err) *err << "Procedure phase 6 should restore custom persistable steps through the local loader registry\n";
      std::filesystem::remove_all(bundle);
      return false;
    }

    const auto artifact = bundle / "steps" / "0_scale";
    {
      std::ofstream out_corrupt(artifact, std::ios::trunc);
      out_corrupt << "999\n";
    }
    bool checksum_failed = false;
    try {
      (void)ai::procedure::load(bundle.string());
    } catch (const std::exception& e) {
      checksum_failed = std::string(e.what()).find("checksum mismatch") != std::string::npos;
    }
    if (!checksum_failed) {
      if (err) *err << "Procedure phase 6 should reject corrupted persisted component artifacts via checksum validation\n";
      std::filesystem::remove_all(bundle);
      return false;
    }
    std::filesystem::remove_all(bundle);
  }

  {
    const auto bundle_a = fresh_bundle_path("procedure_phase6_lymph_codec");
    const auto bundle_b = fresh_bundle_path("procedure_phase6_lymph_codec_resave");
    const lymph::TrainOptions train = rich_train_options();
    const lymph::InferOptions infer = rich_infer_options();
    const lymph::AssessOptions assess = rich_assess_options();
    lymph::models::Model trained_model = lymph::models::decision_tree_regressor({.max_depth = 3});
    trained_model.learn_(regression_batch(), lymph::TrainOptions{});
    lymph::workflow::Workflow trained_workflow =
        lymph::workflow::make(lymph::models::decision_tree_regressor({.max_depth = 2}));
    trained_workflow.learn_(regression_batch(), lymph::TrainOptions{});
    const ai::Procedure proc = ai::procedure::make({
        ai::procedure::step("model_step", "train", "model_out",
                            std::move(trained_model),
                            train, infer, assess),
        ai::procedure::step("workflow_step", "train", "workflow_out",
                            std::move(trained_workflow),
                            train, infer, assess),
    });
    proc.save(bundle_a.string());

    const ai::Procedure restored = ai::procedure::load(bundle_a.string());
    if (!restored.get("model_step").reloadable() || !restored.get("workflow_step").reloadable()) {
      if (err) *err << "Procedure phase 6 should treat Lymph model/workflow steps with serialized option codecs as reloadable\n";
      std::filesystem::remove_all(bundle_a);
      std::filesystem::remove_all(bundle_b);
      return false;
    }
    if (restored.get("model_step").persist_fields() != proc.get("model_step").persist_fields() ||
        restored.get("workflow_step").persist_fields() != proc.get("workflow_step").persist_fields()) {
      if (err) *err << "Procedure phase 6 should restore non-default Lymph option manifests without drift\n";
      std::filesystem::remove_all(bundle_a);
      std::filesystem::remove_all(bundle_b);
      return false;
    }

    restored.save(bundle_b.string());
    const std::vector<std::string> suffixes = {
        ".train_sample_weights.bin",
        ".train_class_weights.bin",
        ".train_parameter_lower_bounds.bin",
        ".train_parameter_upper_bounds.bin",
        ".train_balance_counts.bin",
        ".assess_sample_weights.bin",
    };
    for (const std::string& suffix : suffixes) {
      if (!equal_series_file(bundle_a / "steps" / ("0_model_step" + suffix),
                             bundle_b / "steps" / ("0_model_step" + suffix)) ||
          !equal_series_file(bundle_a / "steps" / ("1_workflow_step" + suffix),
                             bundle_b / "steps" / ("1_workflow_step" + suffix))) {
        if (err) *err << "Procedure phase 6 should roundtrip Lymph option sidecar Series exactly across save-load-save\n";
        std::filesystem::remove_all(bundle_a);
        std::filesystem::remove_all(bundle_b);
        return false;
      }
    }
    std::filesystem::remove_all(bundle_a);
    std::filesystem::remove_all(bundle_b);
  }

  {
    const auto bundle_a = fresh_bundle_path("procedure_phase6_ice_codec");
    const auto bundle_b = fresh_bundle_path("procedure_phase6_ice_codec_resave");
    const ai::Step ice_step = ice_outline_step();
    const ai::Procedure proc = ai::procedure::make({ice_step});
    proc.save(bundle_a.string());

    const ai::Procedure restored = ai::procedure::load(bundle_a.string());
    if (!restored.get("ice_model").reloadable()) {
      if (err) *err << "Procedure phase 6 should reload explicitly bound Ice model steps as reloadable artifacts\n";
      std::filesystem::remove_all(bundle_a);
      std::filesystem::remove_all(bundle_b);
      return false;
    }
    if (restored.get("ice_model").persist_fields() != proc.get("ice_model").persist_fields()) {
      if (err) *err << "Procedure phase 6 should roundtrip Ice train/assess option fields without drift\n";
      std::filesystem::remove_all(bundle_a);
      std::filesystem::remove_all(bundle_b);
      return false;
    }

    const ai::ProcedureResult baseline = proc.run({ai::procedure::input("train", tiny_ice_batch().features)});
    const ai::ProcedureResult roundtrip = restored.run({ai::procedure::input("train", tiny_ice_batch().features)});
    if (as_vec(baseline.get<Series>("ice_out")) != as_vec(roundtrip.get<Series>("ice_out"))) {
      if (err) *err << "Procedure phase 6 should restore Ice model weights faithfully enough to preserve inference outputs\n";
      std::filesystem::remove_all(bundle_a);
      std::filesystem::remove_all(bundle_b);
      return false;
    }

    restored.save(bundle_b.string());
    const ai::Procedure restored_again = ai::procedure::load(bundle_b.string());
    if (restored_again.get("ice_model").persist_fields() != restored.get("ice_model").persist_fields()) {
      if (err) *err << "Procedure phase 6 should keep Ice manifest fields stable across save-load-save\n";
      std::filesystem::remove_all(bundle_a);
      std::filesystem::remove_all(bundle_b);
      return false;
    }

    std::filesystem::path manifest = bundle_a / "manifest.plussproc";
    {
      std::ifstream in(manifest);
      if (!in) {
        if (err) *err << "Procedure phase 6 Ice contract test could not reopen its manifest\n";
        std::filesystem::remove_all(bundle_a);
        std::filesystem::remove_all(bundle_b);
        return false;
      }
      std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
      const std::string needle = "component_schema=1\n";
      const std::size_t pos = text.find(needle);
      if (pos == std::string::npos) {
        if (err) *err << "Procedure phase 6 Ice contract test could not find component schema in manifest\n";
        std::filesystem::remove_all(bundle_a);
        std::filesystem::remove_all(bundle_b);
        return false;
      }
      text.replace(pos, needle.size(), "component_schema=99\n");
      std::ofstream out(manifest, std::ios::trunc);
      out << text;
    }
    bool mismatch_failed = false;
    try {
      (void)ai::procedure::load(bundle_a.string());
    } catch (const std::exception& e) {
      mismatch_failed = std::string(e.what()).find("component schema mismatch") != std::string::npos;
    }
    if (!mismatch_failed) {
      if (err) *err << "Procedure phase 6 should reject family artifact schema drift during reload\n";
      std::filesystem::remove_all(bundle_a);
      std::filesystem::remove_all(bundle_b);
      return false;
    }

    std::filesystem::remove_all(bundle_a);
    std::filesystem::remove_all(bundle_b);
  }

  {
    const ai::Procedure nonpersist = ai::procedure::make({
        ai::procedure::step("twice", "x", "y", [](const Series& input) {
          return input.mul(2.0);
        }),
    });
    bool threw = false;
    try {
      nonpersist.save(fresh_bundle_path("procedure_phase6_nonpersist").string());
    } catch (const std::exception& e) {
      threw = std::string(e.what()).find("not persistable") != std::string::npos;
    }
    if (!threw) {
      if (err) *err << "Procedure phase 6 should fail loudly when a callable-only step has no persistence contract\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("Procedure phase 6 persistence and registry hooks", "[procedure][persist]") {
  REQUIRE(run_phase6_checks(&std::cerr));
}
#else
int main() {
  if (!run_phase6_checks(&std::cerr)) return 1;
  std::cout << "procedure phase 6 checks passed\n";
  return 0;
}
#endif
