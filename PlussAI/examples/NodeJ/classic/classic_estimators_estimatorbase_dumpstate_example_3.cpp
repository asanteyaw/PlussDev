// estimators::EstimatorBase::dump_state example 3 -- on an untrained
// estimator.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});

  const cls::StatusOr<std::vector<pluss::Series>> dumped = made.value()->dump_state();
  std::cout << "trained() = " << std::boolalpha << made.value()->trained() << "\n";
  std::cout << "dump_state ok = " << dumped.ok() << "\n";
  std::cout << "tensor count = " << dumped.value().size() << "\n";

  return 0;
}
