#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;

  // branch(name, columns, steps) just builds the WorkflowBranchSpec value
  // consumed by the make(...) overloads above -- inspecting it directly
  // shows exactly what it holds.
  const workflow::WorkflowBranchSpec spec = workflow::branch(
      "numeric", {"income", "balance"},
      {workflow::WorkflowStepSpec{"scale", lymph::prep::stage::standard_scaler()},
       workflow::WorkflowStepSpec{"impute", lymph::prep::stage::simple_imputer()}});

  std::cout << "name = " << spec.name << "  columns = " << spec.columns.size()
            << "  steps = " << spec.steps.size() << "\n";
  for (const auto& step : spec.steps) {
    std::cout << "  step name = " << step.name << "\n";
  }

  return 0;
}
