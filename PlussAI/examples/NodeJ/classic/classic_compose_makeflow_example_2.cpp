// compose::make_flow example 2 -- make_workflow, the alias spelling.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::StatusOr<cls::Workflow> made = cls::make_workflow(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}});
  std::cout << "made.ok() = " << std::boolalpha << made.ok() << "\n";

  cls::Flow flow = made.move_value();  // classic::Workflow is classic::Flow
  std::cout << "task_kind() == Regression: " << (flow.task_kind() == cls::TaskKind::Regression) << "\n";

  return 0;
}
