// estimators::EstimatorBase::infer example 2 -- continuous predictions from a
// trained regressor, on unseen data.

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

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({5.0, 6.0}, {2}, f64).reshape({2, 1});
  const cls::StatusOr<pluss::Series> pred = made.value()->infer(probe);
  std::cout << "predictions for x = [5, 6] = " << pred.value() << "\n";

  return 0;
}
