// core::AssessOptions example 1 -- assess with an explicit metric list.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.2, 1.4, 1.6, 2.0, 2.2, 2.4, 2.6}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;  // classification needs Logistic or Hinge
  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  made.value()->learn(batch);

  cls::AssessOptions opts{};
  opts.metrics = {cls::MetricKind::Accuracy, cls::MetricKind::F1, cls::MetricKind::RocAuc};

  const cls::StatusOr<cls::Series> scored = made.value()->assess(batch, opts);
  std::cout << "assess ok = " << std::boolalpha << scored.ok() << "\n";
  std::cout << "values = " << scored.value() << "\n";

  return 0;
}
