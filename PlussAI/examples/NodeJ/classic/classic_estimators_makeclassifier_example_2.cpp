// estimators::make_classifier example 2 -- untrained right after
// construction, trained after learn().

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Hinge;
  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  std::cout << "made.ok() = " << std::boolalpha << made.ok() << "\n";
  std::cout << "trained() right after construction = " << made.value()->trained() << "\n";

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  batch.y = pluss::make_series({0.0, 1.0}, {2}, f64);
  made.value()->learn(batch);
  std::cout << "trained() after learn()            = " << made.value()->trained() << "\n";

  return 0;
}
