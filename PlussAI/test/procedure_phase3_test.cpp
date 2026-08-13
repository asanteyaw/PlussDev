#include <pluss/ai/procedure.h>
#include <pluss/ai/lymph.h>
#include <pluss/mic.h>
#include <pluss/pluss.h>

#include <iostream>
#include <string>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

namespace ai = pluss::ai;
namespace lymph = pluss::ai::lymph;

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::table::DataFrame;
using pluss::table::Thread;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
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

DataFrame raw_frame() {
  return DataFrame({
      {"id", Thread(pluss::make_series(std::vector<double>{100, 101, 102, 103}, {4}, f64()))},
      {"timestamp", Thread(pluss::make_series(std::vector<double>{1, 2, 3, 4}, {4}, f64()))},
      {"x", Thread(pluss::make_series(std::vector<double>{0, 1, 2, 3}, {4}, f64()))},
      {"y", Thread(pluss::make_series(std::vector<double>{0, 2, 4, 6}, {4}, f64()))},
  });
}

bool run_phase3_checks(std::ostream* err = nullptr) {
  {
    lymph::Payload payload = lymph::payload::make(raw_frame())
        .features({"x"})
        .label("y")
        .time(lymph::TimeOptions{.index = "timestamp", .horizon = 1});
    const ai::Procedure proc = ai::procedure::make({
        ai::procedure::step("gate", "raw", "clean", lymph::Policy{
            .drop_columns = {"id"},
            .require_columns = {"timestamp", "x", "y"},
            .reject_missing = {"timestamp", "y"},
        }),
    });
    const ai::ProcedureResult result = proc.run({ai::procedure::input("raw", payload)});
    const lymph::Payload clean = result.get<lymph::Payload>("clean");
    if (!clean.has_time() || clean.time_feature() != "timestamp" || clean.frame().get("id").has_value()) {
      if (err) *err << "Procedure Policy adapter should apply raw admission while preserving Payload time roles\n";
      return false;
    }
  }

  {
    lymph::TrainOptions train{};
    train.max_iterations = 50;
    lymph::AssessOptions assess{};
    assess.metrics = {lymph::MetricKind::Mse, lymph::MetricKind::R2};

    lymph::Model tree = lymph::models::decision_tree_regressor({.max_depth = 3});
    const ai::Procedure learn_proc = ai::procedure::make({
        ai::procedure::step("learn_tree", "train", "model", tree, train),
    });
    ai::RunOptions learn_mode{};
    learn_mode.mode = ai::ProcedureMode::Learn;
    const ai::ProcedureResult learned = learn_proc.run({ai::procedure::input("train", regression_batch())}, learn_mode);
    const lymph::Model model = learned.get<lymph::Model>("model");
    if (!model.trained()) {
      if (err) *err << "Procedure Model adapter should return a trained Model in Learn mode\n";
      return false;
    }

    const ai::Procedure infer_proc = ai::procedure::make({
        ai::procedure::step("infer_tree", "features", "pred", model),
    });
    const ai::ProcedureResult inferred = infer_proc.run({
        ai::procedure::input("features", regression_batch().features),
    });
    const Series pred = inferred.get<Series>("pred");
    if (pred.numel() != 6) {
      if (err) *err << "Procedure Model adapter should infer one prediction per row\n";
      return false;
    }

    ai::RunOptions assess_mode{};
    assess_mode.mode = ai::ProcedureMode::Assess;
    const ai::Procedure assess_proc = ai::procedure::make({
        ai::procedure::step("assess_tree", "eval", "report", model, train, lymph::InferOptions{}, assess),
    });
    const ai::ProcedureResult assessed = assess_proc.run({ai::procedure::input("eval", regression_batch())}, assess_mode);
    const lymph::MetricReport report = assessed.get<lymph::MetricReport>("report");
    if (report.values.numel() != 2 || report.requested.size() != 2) {
      if (err) *err << "Procedure Model adapter should return requested Lymph MetricReport values in Assess mode\n";
      return false;
    }
  }

  {
    lymph::Workflow wf = lymph::workflow::make(lymph::models::decision_tree_regressor({.max_depth = 3}));
    const ai::Procedure learn_proc = ai::procedure::make({
        ai::procedure::step("learn_wf", "train", "workflow", wf),
    });
    ai::RunOptions learn_mode{};
    learn_mode.mode = ai::ProcedureMode::Learn;
    const ai::ProcedureResult learned = learn_proc.run({ai::procedure::input("train", regression_batch())}, learn_mode);
    const lymph::Workflow trained = learned.get<lymph::Workflow>("workflow");
    if (!trained.trained()) {
      if (err) *err << "Procedure Workflow adapter should return a trained Workflow in Learn mode\n";
      return false;
    }

    const ai::Procedure infer_proc = ai::procedure::make({
        ai::procedure::step("infer_wf", "features", "pred", trained),
        ai::procedure::step("out", ai::procedure::sink("phase3.out")),
    }, {
        ai::procedure::link("infer_wf", "out"),
    });
    const ai::ProcedureResult inferred = infer_proc.run({ai::procedure::input("features", regression_batch().features)});
    if (inferred.get<Series>("pred").numel() != 6 || inferred.get<Series>("out").numel() != 6) {
      if (err) *err << "Procedure Workflow adapter should infer and flow predictions through a sink\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("Procedure phase 3 Lymph adapters", "[procedure][lymph]") {
  REQUIRE(run_phase3_checks(&std::cerr));
}
#else
int main() {
  if (!run_phase3_checks(&std::cerr)) return 1;
  std::cout << "procedure phase 3 checks passed\n";
  return 0;
}
#endif
