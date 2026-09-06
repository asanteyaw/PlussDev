#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;

  // make(branches, model, options) -- Policy defaults to Policy{} when the
  // caller has nothing to admit/reject and only wants the declarative
  // branch DSL plus worker count.
  const std::vector<workflow::WorkflowBranchSpec> branches = {
      workflow::branch("numeric", {"income", "balance"},
                       {workflow::WorkflowStepSpec{"scale", lymph::prep::stage::standard_scaler()}}),
  };

  workflow::WorkflowOptions options{};
  options.workers = 2;

  const workflow::Workflow wf = workflow::make(branches, lymph::models::logistic_regression(), options);
  std::cout << "policy().empty() = " << std::boolalpha << wf.policy().empty()
            << "  workers = " << wf.workers()
            << "  branch_count = " << wf.branch_count() << "\n";

  return 0;
}
