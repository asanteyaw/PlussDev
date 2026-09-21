// estimators::EstimatorBase::learn example 3 -- a shape-mismatched batch.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  batch.y = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64);  // 3 labels, 4 samples

  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made =
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                         cls::LinearFamilyOptions{}});
  const cls::Status learned = made.value()->learn(batch);
  std::cout << "ok = " << std::boolalpha << learned.ok() << "\n";
  std::cout << "message = \"" << learned.message << "\"\n";
  std::cout << "trained() after the failed fit = " << made.value()->trained() << "\n";

  return 0;
}
