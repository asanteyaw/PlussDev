#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  // The declarative branch-spec DSL: every branch built in one make(...)
  // call from workflow::branch(name, columns, steps) values, instead of
  // chaining .on(...).add(...) calls -- useful when the branch layout is
  // itself data (e.g. assembled from a config file).
  const std::vector<workflow::WorkflowBranchSpec> branches = {
      workflow::branch("numeric", {"income", "balance"},
                       {workflow::WorkflowStepSpec{"scale", lymph::prep::stage::standard_scaler()}}),
  };

  const workflow::Workflow wf = workflow::make(workflow::Policy{}, branches, lymph::models::logistic_regression());
  std::cout << "branch_count = " << wf.branch_count() << "  joined = " << std::boolalpha << wf.joined() << "\n";

  const workflow::Workflow trained = wf.learn(raw, data_options);
  std::cout << "stage_count = " << trained.stage_count() << "\n";
  std::cout << "pred = " << trained.infer(raw, data_options) << "\n";

  return 0;
}
