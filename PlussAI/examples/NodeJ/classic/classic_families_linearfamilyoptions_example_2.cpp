// families::LinearFamilyOptions example 2 -- RobustLeast vs LeastSquares
// against an outlier.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // y = 2x, except the last point is a wild outlier.
  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {6}, f64).reshape({6, 1});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0, 10.0, 40.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({7.0}, {1}, f64).reshape({1, 1});

  auto fit_and_predict = [&](cls::LinearObjective objective) {
    cls::LinearFamilyOptions opts{};
    opts.objective = objective;
    opts.regularization_strength = 1e-6;
    cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made =
        cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, opts});
    made.value()->learn(batch);
    return made.value()->infer(probe).value().item<double>();
  };

  std::cout << "true trend at x = 7 (ignoring the outlier): 14\n";
  std::cout << "LeastSquares prediction : " << fit_and_predict(cls::LinearObjective::LeastSquares) << "\n";
  std::cout << "RobustLeast prediction  : " << fit_and_predict(cls::LinearObjective::RobustLeast) << "\n";

  return 0;
}
