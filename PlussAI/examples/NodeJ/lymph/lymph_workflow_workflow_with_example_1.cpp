#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Slate(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Slate(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  // A "design" Workflow can be built policy/route-first, with no Model
  // attached at all (workflow::make(policy) starts from an empty template).
  workflow::Workflow design = workflow::make(workflow::Policy{});
  design.on({"income", "balance"}).add(lymph::prep::stage::standard_scaler());
  std::cout << "design.has_model() = " << std::boolalpha << design.has_model() << "\n";

  // with(model) replays the recorded route onto a real model, without
  // mutating `design` itself.
  const workflow::Workflow candidate = design.with(lymph::models::logistic_regression());
  std::cout << "candidate.has_model() = " << candidate.has_model()
            << "  candidate.branch_count() = " << candidate.branch_count() << "\n";

  const workflow::Workflow trained = candidate.learn(raw, data_options);
  std::cout << "trained.stage_count() = " << trained.stage_count() << "\n";
  std::cout << "pred = " << trained.infer(raw, data_options) << "\n";

  return 0;
}
