// estimators::EstimatorBase::task_kind example 1 -- classifier vs regressor.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> classifier =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> regressor =
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                         cls::LinearFamilyOptions{}});

  std::cout << "classifier task_kind == Classification: " << std::boolalpha
            << (classifier.value()->task_kind() == cls::TaskKind::Classification) << "\n";
  std::cout << "regressor task_kind == Regression: "
            << (regressor.value()->task_kind() == cls::TaskKind::Regression) << "\n";

  return 0;
}
