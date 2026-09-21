// compose::Flow::set_regressor example 1 -- attaching an already-built
// RegressorBase directly.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};  // default objective (LeastSquares) is fine for regression
  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> model_or =
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, linear});

  cls::Flow flow{};
  const cls::Status s = flow.set_regressor(model_or.move_value());
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "has_model() = " << flow.has_model() << "\n";
  std::cout << "task_kind() == Regression: " << (flow.task_kind() == cls::TaskKind::Regression) << "\n";

  return 0;
}
