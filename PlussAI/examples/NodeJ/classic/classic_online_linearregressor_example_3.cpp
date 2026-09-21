#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64).reshape({5, 1});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0, 10.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::LeastSquares;

  cls::online::OnlineLinearRegressor one_step(options);
  cls::TrainOptions one{};
  one.max_iterations = 1;
  one_step.learn(batch, one);
  cls::AssessOptions r2_only{};
  r2_only.metrics = {cls::MetricKind::R2};
  std::cout << "R2 after max_iterations=1    = " << one_step.assess(batch, r2_only).value() << "\n";

  cls::online::OnlineLinearRegressor default_fit(options);
  default_fit.learn(batch);  // max_iterations defaults to 1000
  std::cout << "R2 after max_iterations=1000 = " << default_fit.assess(batch, r2_only).value() << "\n";

  return 0;
}
