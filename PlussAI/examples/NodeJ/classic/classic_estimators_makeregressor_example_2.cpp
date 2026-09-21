// estimators::make_regressor example 2 -- build, fit, predict in one line.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made = cls::make_regressor(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}});

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  batch.y = pluss::make_series({2.0, 4.0, 6.0}, {3}, f64);
  made.value()->learn(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({4.0}, {1}, f64).reshape({1, 1});
  std::cout << "prediction = " << made.value()->infer(probe).value().item<double>() << "\n";

  return 0;
}
