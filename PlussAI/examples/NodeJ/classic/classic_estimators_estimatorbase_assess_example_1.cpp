// estimators::EstimatorBase::assess example 1 -- the task-default metric set.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made =
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                         cls::LinearFamilyOptions{}});
  made.value()->learn(batch);

  const cls::StatusOr<pluss::Series> scored = made.value()->assess(batch);
  std::cout << "ok = " << std::boolalpha << scored.ok() << "\n";
  std::cout << "default metrics = " << scored.value() << "\n";

  return 0;
}
