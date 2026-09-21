#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::Logistic;
  cls::online::OnlineLinearClassifier model(options);

  const cls::Series x = pluss::make_series(
      {-1.0, -0.8, -0.6, -0.4, 0.4, 0.6, 0.8, 1.0}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  model.learn(batch);

  std::cout << "default {Accuracy}       = " << model.assess(batch).value() << "\n";

  cls::AssessOptions multi{};
  multi.metrics = {cls::MetricKind::F1, cls::MetricKind::Mcc};
  std::cout << "[F1, Mcc]                = " << model.assess(batch, multi).value() << "\n";

  cls::AssessOptions unsupported{};
  unsupported.metrics = {cls::MetricKind::RocAuc};
  const cls::StatusOr<pluss::Series> rejected = model.assess(batch, unsupported);
  std::cout << "RocAuc ok = " << std::boolalpha << rejected.ok() << "\n";
  std::cout << "message = \"" << rejected.status().message << "\"\n";

  return 0;
}
