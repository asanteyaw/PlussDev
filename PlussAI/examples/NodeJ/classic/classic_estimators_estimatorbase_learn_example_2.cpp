// estimators::EstimatorBase::learn example 2 -- an explicit TrainOptions, and
// re-fitting the same estimator (a fresh fit, not a warm-started refinement).

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  const cls::Series y = pluss::make_series({2.1, 3.9, 6.1, 7.9}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made =
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                         cls::LinearFamilyOptions{}});

  cls::TrainOptions opts{};
  opts.max_iterations = 500;
  const cls::Status first = made.value()->learn(batch, opts);
  std::cout << "first learn ok  = " << std::boolalpha << first.ok() << "\n";

  const cls::Status second = made.value()->learn(batch, opts);  // re-fits from scratch
  std::cout << "second learn ok = " << second.ok() << "\n";

  return 0;
}
