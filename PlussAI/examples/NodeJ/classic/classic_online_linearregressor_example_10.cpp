#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::LeastSquares;
  cls::online::OnlineLinearRegressor model(options);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64).reshape({5, 1});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0, 10.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  model.learn(batch);

  std::cout << "default [Mse, Rmse, R2] = " << model.assess(batch).value() << "\n";

  cls::AssessOptions mae_only{};
  mae_only.metrics = {cls::MetricKind::Mae};
  std::cout << "[Mae]                   = " << model.assess(batch, mae_only).value() << "\n";

  cls::AssessOptions unsupported{};
  unsupported.metrics = {cls::MetricKind::QuantileLoss};
  const cls::StatusOr<pluss::Series> rejected = model.assess(batch, unsupported);
  std::cout << "QuantileLoss ok = " << std::boolalpha << rejected.ok() << "\n";
  std::cout << "message = \"" << rejected.status().message << "\"\n";

  return 0;
}
