#include <pluss/ai/procedure.h>
#include <pluss/mic.h>
#include <pluss/nlp.h>
#include <pluss/pluss.h>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

namespace ai = pluss::ai;
namespace ice = pluss::ai::ice;
namespace lymph = pluss::ai::lymph;
namespace nodeb = pluss::ai::nodeb;

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

Config f32() {
  return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
}

Series vec(std::vector<float> values) {
  return pluss::make_series(std::move(values), {static_cast<std::int64_t>(values.size()), 1}, f32());
}

lymph::LabeledBatch tiny_labeled_batch() {
  const Series x = vec({1.0F, 2.0F, 3.0F, 4.0F});
  const Series y = vec({2.0F, 4.0F, 6.0F, 8.0F});
  return lymph::LabeledBatch{
      .features = lymph::FeatureBatch{.x = x, .layout = lymph::ObservationLayout::Rows},
      .y = y,
  };
}

bool close_vec(const Series& values, const std::vector<double>& want, double tol = 1e-6) {
  const std::vector<double> got =
      values.to(pluss::pCPU, ScalarType::Float64).reshape({values.numel()}).to_vec<double>();
  if (got.size() != want.size()) return false;
  for (std::size_t i = 0; i < got.size(); ++i) {
    if (std::abs(got[i] - want[i]) > tol) return false;
  }
  return true;
}

bool run_phase4_checks(std::ostream* err = nullptr) {
  {
    auto model = ice::Outline(pluss::nn::Fusion(1, 1));
    ice::TrainOptions train{};
    train.loss = ice::LossKind::MSE;
    train.execution.max_epochs = 2;
    train.validation.split_ratio = 0.0;
    train.batch_size = 4;
    train.learning_rate = 0.01;

    const ai::Step step =
        ai::procedure::step("ice_model", "x", "y", static_cast<ice::Model&>(model), ice::LossKind::MSE, train);
    if (!step.has(ai::StepCapability::Infer) ||
        !step.has(ai::StepCapability::Learn) ||
        !step.has(ai::StepCapability::Assess) ||
        step.persistable()) {
      if (err) *err << "Procedure Ice adapter should advertise infer/learn/assess and remain non-persistable by default\n";
      return false;
    }

    const ai::Procedure learn_proc = ai::procedure::make({
        ai::procedure::step("learn_ice", "train", "learned", static_cast<ice::Model&>(model), ice::LossKind::MSE, train),
    });
    ai::RunOptions learn_mode{};
    learn_mode.mode = ai::ProcedureMode::Learn;
    const ai::ProcedureResult learned = learn_proc.run({ai::procedure::input("train", tiny_labeled_batch())}, learn_mode);
    if (!learned.get<bool>("learned") || !model.ready()) {
      if (err) *err << "Procedure Ice adapter should delegate Learn mode to the referenced Ice model owner\n";
      return false;
    }

    const ai::Procedure proc = ai::procedure::make({step});
    const ai::ProcedureResult result = proc.run({ai::procedure::input("x", vec({1.0F, 2.0F, 3.0F}))});
    if (result.get<Series>("y").numel() != 3) {
      if (err) *err << "Procedure Ice adapter should infer directly from trained Series inputs\n";
      return false;
    }

    ice::Batch batch{};
    batch.set_inputs(vec({2.0F, 4.0F}));
    const ai::ProcedureResult batch_result = proc.run({ai::procedure::input("x", batch)});
    if (batch_result.get<Series>("y").numel() != 2) {
      if (err) *err << "Procedure Ice adapter should infer from ice::Batch inputs without row plumbing\n";
      return false;
    }

    ai::RunOptions assess_mode{};
    assess_mode.mode = ai::ProcedureMode::Assess;
    const ai::Procedure assess_proc = ai::procedure::make({
        ai::procedure::step("assess_ice", "eval", "report", static_cast<ice::Model&>(model), ice::LossKind::MSE),
    });
    const ai::ProcedureResult assessed =
        assess_proc.run({ai::procedure::input("eval", tiny_labeled_batch())}, assess_mode);
    const ice::AssessReport report = assessed.get<ice::AssessReport>("report");
    if (report.metric_name != "rmse" || report.metric <= 0.0 || report.loss <= 0.0) {
      if (err) *err << "Procedure Ice adapter should return an Ice AssessReport in Assess mode\n";
      return false;
    }
  }

  {
    pluss::nn::ReLU relu{};
    const ai::Procedure proc = ai::procedure::make({
        ai::procedure::step("relu", "x", "y", relu),
    });
    const ai::ProcedureResult result = proc.run({ai::procedure::input("x", vec({-2.0F, 0.0F, 3.0F}))});
    if (!close_vec(result.get<Series>("y"), {0.0, 0.0, 3.0})) {
      if (err) *err << "Procedure raw NodeK escape hatch should run vectorized Series module calls\n";
      return false;
    }
  }

  {
    nodeb::Workflow nlp{
        .lang = nodeb::Language::English,
        .stages = {
            nodeb::stage::tokenize(),
            nodeb::stage::sentencize(),
            nodeb::stage::lemmatize(),
        },
    };
    const std::vector<std::string> texts = {
        "Dr. Laurel writes APIs. Second sentence!",
        "C++ tests should stay small. They should still prove behavior.",
    };

    const ai::Step step = ai::procedure::step("nlp", "text", "docs", nlp);
    if (!step.persistable() ||
        !step.has(ai::StepCapability::Infer) ||
        !step.has(ai::StepCapability::Learn) ||
        !step.has(ai::StepCapability::Assess)) {
      if (err) *err << "Procedure NodeB adapter should advertise persistable workflow capabilities\n";
      return false;
    }

    const ai::Procedure infer_proc = ai::procedure::make({
        step,
        ai::procedure::step("out", ai::procedure::sink("phase4.docs")),
    }, {
        ai::procedure::link("nlp", "out"),
    });
    const ai::ProcedureResult inferred = infer_proc.run({ai::procedure::input("text", texts)});
    const std::vector<nodeb::Doc> docs = inferred.get<std::vector<nodeb::Doc>>("docs");
    if (docs.size() != 2 ||
        docs[0].sentences.size() != 2 ||
        docs[0].lemma_texts().empty() ||
        inferred.get<std::vector<nodeb::Doc>>("out").size() != 2) {
      if (err) *err << "Procedure NodeB adapter should infer Doc vectors and pass them through sinks\n";
      return false;
    }

    const ai::Procedure one_proc = ai::procedure::make({
        ai::procedure::step("one_doc", "text", "doc", nlp),
    });
    const ai::ProcedureResult one = one_proc.run({ai::procedure::input("text", std::string("One short sentence."))});
    if (one.get<nodeb::Doc>("doc").tokens.empty()) {
      if (err) *err << "Procedure NodeB adapter should infer a single Doc from a single string\n";
      return false;
    }

    ai::RunOptions assess_mode{};
    assess_mode.mode = ai::ProcedureMode::Assess;
    const ai::Procedure assess_proc = ai::procedure::make({
        ai::procedure::step("assess_nlp", "text", "report", nlp),
    });
    const ai::ProcedureResult assessed = assess_proc.run({ai::procedure::input("text", texts)}, assess_mode);
    const nodeb::WorkflowReport report = assessed.get<nodeb::WorkflowReport>("report");
    if (report.doc_count != 2 || report.sentence_count != 4 || report.token_count <= 0) {
      if (err) *err << "Procedure NodeB adapter should return WorkflowReport counts in Assess mode\n";
      return false;
    }

    ai::RunOptions learn_mode{};
    learn_mode.mode = ai::ProcedureMode::Learn;
    const ai::Procedure learn_proc = ai::procedure::make({
        ai::procedure::step("learn_nlp", "text", "workflow", nlp),
    });
    const ai::ProcedureResult learned = learn_proc.run({ai::procedure::input("text", texts)}, learn_mode);
    if (learned.get<nodeb::Workflow>("workflow").stages.size() != 3) {
      if (err) *err << "Procedure NodeB adapter should return the learned Workflow artifact\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("Procedure phase 4 Ice and NodeB adapters", "[procedure][ice][nodeb]") {
  REQUIRE(run_phase4_checks(&std::cerr));
}
#else
int main() {
  if (!run_phase4_checks(&std::cerr)) return 1;
  std::cout << "procedure phase 4 checks passed\n";
  return 0;
}
#endif
