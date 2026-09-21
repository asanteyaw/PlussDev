// estimators::ClassifierBase example 1 -- infer_scores alongside infer.

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
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  made.value()->learn(batch);

  cls::FeatureBatch probes{};
  probes.x = pluss::make_series({1.0, 1.8, 2.6}, {3}, f64).reshape({3, 1});

  std::cout << "task_kind() == Classification: " << std::boolalpha
            << (made.value()->task_kind() == cls::TaskKind::Classification) << "\n";
  std::cout << "raw scores  = " << made.value()->infer_scores(probes).value() << "\n";
  std::cout << "hard labels = " << made.value()->infer(probes).value() << "\n";

  return 0;
}
