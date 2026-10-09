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

  // Swap the base model out, keeping the same routed prep -- useful for a
  // search/spotcheck-style factory that reuses one Workflow's routing
  // across several candidate model families.
  workflow::Workflow logistic_wf = workflow::make(lymph::models::logistic_regression());
  logistic_wf.on({"income", "balance"}).add(lymph::prep::stage::standard_scaler());

  const workflow::Workflow tree_wf = logistic_wf.with(lymph::models::tree::decision_tree_classifier());
  std::cout << "tree_wf.branch_count() = " << tree_wf.branch_count() << "\n";

  const workflow::Workflow trained_logistic = logistic_wf.learn(raw, data_options);
  const workflow::Workflow trained_tree = tree_wf.learn(raw, data_options);
  std::cout << "logistic task_kind == tree task_kind: "
            << std::boolalpha << (trained_logistic.task_kind() == trained_tree.task_kind()) << "\n";

  return 0;
}
