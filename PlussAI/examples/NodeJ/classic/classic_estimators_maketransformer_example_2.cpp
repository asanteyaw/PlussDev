// estimators::make_transformer example 2 -- .task is checked, and so is
// .family (Linear has no transformer form at all).

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec wrong_task{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear};

  const cls::StatusOr<std::unique_ptr<cls::TransformerBase>> made = cls::make_transformer(wrong_task);
  std::cout << "ok = " << std::boolalpha << made.ok() << "\n";
  std::cout << "message = \"" << made.status().message << "\"\n";

  const cls::ModelSpec right_task_wrong_family{cls::TaskKind::Transform, cls::ModelFamily::Linear, linear};
  const cls::StatusOr<std::unique_ptr<cls::TransformerBase>> made2 = cls::make_transformer(right_task_wrong_family);
  std::cout << "task fixed, family still Linear: ok = " << made2.ok() << ", message = \""
            << made2.status().message << "\"\n";

  return 0;
}
