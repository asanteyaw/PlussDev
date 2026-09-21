// estimators::EstimatorBase::trained example 3 -- independent per estimator
// instance.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::ModelSpec spec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}};
  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> a = cls::make_regressor(spec);
  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> b = cls::make_regressor(spec);

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  batch.y = pluss::make_series({2.0, 4.0, 6.0}, {3}, f64);
  a.value()->learn(batch);  // only fit "a"

  std::cout << "a.trained() = " << std::boolalpha << a.value()->trained() << "\n";
  std::cout << "b.trained() = " << b.value()->trained() << "\n";

  return 0;
}
