// estimators::EstimatorBase::task_kind example 3 -- branching generically.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  auto describe = [](cls::TaskKind kind) {
    switch (kind) {
      case cls::TaskKind::Classification: return "classification";
      case cls::TaskKind::Regression: return "regression";
      case cls::TaskKind::Transform: return "transform";
    }
    return "unknown";
  };

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Hinge;
  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});

  std::cout << "this estimator solves a " << describe(made.value()->task_kind()) << " problem\n";

  return 0;
}
