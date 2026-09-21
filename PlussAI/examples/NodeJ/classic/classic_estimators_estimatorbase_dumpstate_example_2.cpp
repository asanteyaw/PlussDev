// estimators::EstimatorBase::dump_state example 2 -- the full dump/load round
// trip between two independent classifiers.

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
  const cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear};

  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> original = cls::make_classifier(spec);
  original.value()->learn(batch);
  const std::vector<pluss::Series> state = original.value()->dump_state().value();

  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> restored = cls::make_classifier(spec);
  std::cout << "restored.trained() before load_state = " << std::boolalpha
            << restored.value()->trained() << "\n";
  const cls::Status loaded = restored.value()->load_state(state);
  std::cout << "load_state ok = " << loaded.ok() << "\n";
  std::cout << "restored.trained() after load_state  = " << restored.value()->trained() << "\n";

  const pluss::Series original_pred = original.value()->infer(batch.features).value();
  const pluss::Series restored_pred = restored.value()->infer(batch.features).value();
  std::cout << "original predictions = " << original_pred << "\n";
  std::cout << "restored predictions = " << restored_pred << "\n";

  return 0;
}
