// estimators::EstimatorBase::trained example 2 -- guarding a call that would
// otherwise fail.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made =
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                         cls::LinearFamilyOptions{}});

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0}, {1}, f64).reshape({1, 1});

  if (made.value()->trained()) {
    std::cout << "prediction = " << made.value()->infer(probe).value() << "\n";
  } else {
    std::cout << "skipped infer: estimator is not trained yet\n";
  }

  return 0;
}
