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
      0.0, 0.0,
      1.0, 0.0,
      2.0, 1.0,
      3.0, 1.0,
      4.0, 2.0,
      5.0, 2.0}, {6, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0.0, 1.0, 3.0, 4.0, 6.0, 7.0}, {6}, cfg);
  const lymph::LabeledBatch batch{
      .features = lymph::FeatureBatch{.x = x, .layout = lymph::ObservationLayout::Rows},
      .y = y,
  };

  lymph::Workflow wf = lymph::workflow::make(lymph::models::decision_tree_regressor({.max_depth = 3}));
  ai::RunOptions learn{};
  learn.mode = ai::ProcedureMode::Learn;

  const ai::Procedure trained_proc = ai::procedure::make({
      ai::procedure::step("learn", "train", "workflow", wf),
  });
  const lymph::Workflow trained =
      trained_proc.run({ai::procedure::input("train", batch)}, learn).get<lymph::Workflow>("workflow");

  const ai::Procedure infer_proc = ai::procedure::make({
      ai::procedure::step("infer", "features", "pred", trained),
      ai::procedure::step("out", ai::procedure::sink("procedure.demo.pred")),
  }, {
      ai::procedure::link("infer", "out"),
  });
  const ai::ProcedureResult out = infer_proc.run({ai::procedure::input("features", batch.features)});

  std::cout << "predictions=" << out.get<pluss::Series>("pred").numel() << "\n";
  std::cout << "sink_predictions=" << out.get<pluss::Series>("out").numel() << "\n";
  return 0;
}
