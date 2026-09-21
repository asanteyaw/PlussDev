// estimators::EstimatorBase::assess example 3 -- untrained estimator.

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

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0}, {1}, f64).reshape({1, 1});
  batch.y = pluss::make_series({1.0}, {1}, f64);

  const cls::StatusOr<pluss::Series> scored = made.value()->assess(batch);
  std::cout << "ok = " << std::boolalpha << scored.ok() << "\n";
  std::cout << "message = \"" << scored.status().message << "\"\n";

  return 0;
}
