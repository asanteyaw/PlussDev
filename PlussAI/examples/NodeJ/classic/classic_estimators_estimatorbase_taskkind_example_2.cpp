// estimators::EstimatorBase::task_kind example 2 -- fixed across learn().

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
  const cls::TaskKind before = made.value()->task_kind();

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  batch.y = pluss::make_series({2.0, 4.0}, {2}, f64);
  made.value()->learn(batch);
  const cls::TaskKind after = made.value()->task_kind();

  std::cout << "same before and after learn = " << std::boolalpha << (before == after) << "\n";

  return 0;
}
