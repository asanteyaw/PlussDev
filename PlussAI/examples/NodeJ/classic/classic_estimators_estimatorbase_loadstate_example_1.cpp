// estimators::EstimatorBase::load_state example 1 -- the full round trip.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  const cls::ModelSpec spec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}};
  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> original = cls::make_regressor(spec);
  original.value()->learn(batch);
  const std::vector<pluss::Series> state = original.value()->dump_state().value();

  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> restored = cls::make_regressor(spec);
  const cls::Status loaded = restored.value()->load_state(state);
  std::cout << "load_state ok = " << std::boolalpha << loaded.ok() << "\n";
  std::cout << "restored.trained() = " << restored.value()->trained() << "\n";

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({5.0}, {1}, f64).reshape({1, 1});
  std::cout << "original prediction = " << original.value()->infer(probe).value().item<double>() << "\n";
  std::cout << "restored prediction = " << restored.value()->infer(probe).value().item<double>() << "\n";

  return 0;
}
