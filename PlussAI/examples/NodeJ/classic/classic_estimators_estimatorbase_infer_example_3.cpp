// estimators::EstimatorBase::infer example 3 -- untrained estimator, and a
// feature-count mismatch.

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
  const cls::StatusOr<pluss::Series> untrained = made.value()->infer(probe);
  std::cout << "untrained: ok = " << std::boolalpha << untrained.ok() << ", message = \""
            << untrained.status().message << "\"\n";

  const cls::Series x = pluss::make_series({1.0, 2.0, 1.0, 2.0}, {4}, f64).reshape({2, 2});
  const cls::Series y = pluss::make_series({3.0, 5.0}, {2}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  made.value()->learn(batch);  // trained on 2 features

  cls::FeatureBatch wrong_shape{};
  wrong_shape.x = probe.x;  // only 1 feature
  const cls::StatusOr<pluss::Series> mismatched = made.value()->infer(wrong_shape);
  std::cout << "wrong feature count: ok = " << mismatched.ok() << ", message = \""
            << mismatched.status().message << "\"\n";

  return 0;
}
