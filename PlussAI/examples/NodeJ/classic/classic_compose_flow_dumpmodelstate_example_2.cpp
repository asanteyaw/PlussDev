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
  batch.features.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  batch.y = pluss::make_series({2.0, 4.0, 6.0, 8.0}, {4}, f64);
  original.learn(batch);
  const std::vector<pluss::Series> state = original.dump_model_state().value();

  cls::Flow restored = cls::make_flow(spec).move_value();
  restored.load_model_state(state);
  restored.mark_trained(true);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({5.0}, {1}, f64).reshape({1, 1});
  std::cout << "original predicts = " << original.infer(probe).value().item<double>() << "\n";
  std::cout << "restored predicts = " << restored.infer(probe).value().item<double>() << "\n";

  return 0;
}
