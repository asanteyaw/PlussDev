// compose::Flow::set_classifier example 1 -- attaching an already-built
// ClassifierBase directly, bypassing ModelSpec/make_classifier entirely.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> model_or =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});

  cls::Flow flow{};
  const cls::Status s = flow.set_classifier(model_or.move_value());
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "has_model() = " << flow.has_model() << "\n";
  std::cout << "has_model_spec() = " << flow.has_model_spec() << "\n";
  std::cout << "task_kind() == Classification: " << (flow.task_kind() == cls::TaskKind::Classification) << "\n";

  return 0;
}
