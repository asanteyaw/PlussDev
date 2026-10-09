#include <pluss/ai/procedure.h>
#include <pluss/ai/lymph.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;
  namespace lymph = pluss::ai::lymph;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{
      0.0, 1.0,
      1.0, 2.0,
      2.0, 3.0,
      3.0, 4.0}, {4, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{1.0, 3.0, 5.0, 7.0}, {4}, cfg);
  const lymph::LabeledBatch batch{
      .features = lymph::FeatureBatch{.x = x, .layout = lymph::ObservationLayout::Rows},
      .y = y,
  };

  ai::RunOptions learn{};
  learn.mode = ai::ProcedureMode::Learn;

  const ai::Procedure learner = ai::procedure::make({
      ai::procedure::step("fit", "train", "model", lymph::models::linear_regression()),
  });
  const lymph::models::Model model =
      learner.run({ai::procedure::input("train", batch)}, learn)
          .get<lymph::models::Model>("model");

  const ai::Procedure scorer = ai::procedure::make({
      ai::procedure::step("score", "features", "pred", model),
  });
  const ai::ProcedureResult out = scorer.run({ai::procedure::input("features", batch.features)});

  lymph::AssessOptions assess_options{};
  assess_options.metrics = {lymph::MetricKind::Mse, lymph::MetricKind::R2};
  const ai::Procedure assessor = ai::procedure::make({
      ai::procedure::step(
          "assess",
          "eval",
          "report",
          model,
          lymph::TrainOptions{},
          lymph::InferOptions{},
          assess_options),
  });
  ai::RunOptions assess{};
  assess.mode = ai::ProcedureMode::Assess;
  const lymph::MetricReport report =
      assessor.run({ai::procedure::input("eval", batch)}, assess)
          .get<lymph::MetricReport>("report");

  std::cout << "trained=" << model.trained() << "\n";
  std::cout << "predictions=" << out.get<pluss::Series>("pred").numel() << "\n";
  std::cout << "report_metrics=" << report.values.numel() << "\n";
  return 0;
}
