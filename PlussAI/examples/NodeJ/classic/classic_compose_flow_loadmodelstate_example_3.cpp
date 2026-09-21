#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::ModelSpec spec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}};
  cls::Flow original = cls::make_flow(spec).move_value();
  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  batch.y = pluss::make_series({2.0, 4.0, 6.0}, {3}, f64);
  original.learn(batch);
  const std::vector<pluss::Series> state = original.dump_model_state().value();

  cls::Flow restored = cls::make_flow(spec).move_value();
  const cls::Status loaded = restored.load_model_state(state);
  std::cout << "load ok = " << std::boolalpha << loaded.ok() << "\n";

  return 0;
}
