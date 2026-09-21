// estimators::RegressorBase example 1 -- a minimal regressor lifecycle.

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

  std::cout << "task_kind() == Regression: " << std::boolalpha
            << (made.value()->task_kind() == cls::TaskKind::Regression) << "\n";
  std::cout << "trained before learn      : " << made.value()->trained() << "\n";
  made.value()->learn(batch);
  std::cout << "trained after learn       : " << made.value()->trained() << "\n";

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({5.0}, {1}, f64).reshape({1, 1});
  std::cout << "prediction for x = 5      : " << made.value()->infer(probe).value().item<double>() << "\n";

  return 0;
}
