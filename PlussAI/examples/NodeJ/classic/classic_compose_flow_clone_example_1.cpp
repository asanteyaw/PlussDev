#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}});
  cls::Flow original = made.move_value();
  original.add_standard_scaler();

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  batch.y = pluss::make_series({2.0, 4.0, 6.0, 8.0}, {4}, f64);
  original.learn(batch);

  cls::Flow clone = original.clone().move_value();

  // Reconfigure and re-fit the clone only -- set_nested_param on model::* resets
  // trained() (see set_nested_param's own page), so the clone needs a fresh learn().
  clone.set_nested_param("model::opt.regularization_strength", 1e3);
  clone.learn(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({5.0}, {1}, f64).reshape({1, 1});
  std::cout << "original.trained() = " << std::boolalpha << original.trained() << "\n";
  std::cout << "clone.trained()    = " << clone.trained() << "\n";
  std::cout << "original predicts  = " << original.infer(probe).value().item<double>() << "\n";
  std::cout << "clone predicts     = " << clone.infer(probe).value().item<double>() << "\n";

  return 0;
}
