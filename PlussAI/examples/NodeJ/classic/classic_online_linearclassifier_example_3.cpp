#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {-1.0, -0.8, -0.6, -0.4, 0.4, 0.6, 0.8, 1.0}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::Logistic;
  cls::AssessOptions logloss_only{};
  logloss_only.metrics = {cls::MetricKind::LogLoss};

  cls::online::OnlineLinearClassifier one_step(options);
  cls::TrainOptions one{};
  one.max_iterations = 1;
  one_step.learn(batch, one);
  std::cout << "LogLoss after max_iterations=1    = " << one_step.assess(batch, logloss_only).value() << "\n";

  cls::online::OnlineLinearClassifier default_fit(options);
  default_fit.learn(batch);  // max_iterations defaults to 1000
  std::cout << "LogLoss after max_iterations=1000 = " << default_fit.assess(batch, logloss_only).value() << "\n";

  return 0;
}
