#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}});
  cls::Flow flow = made.move_value();
  flow.add_standard_scaler();

  const cls::Series x = pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4}, f64).reshape({4, 1});
  const cls::Series y = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  flow.learn(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({50.0}, {1}, f64).reshape({1, 1});  // raw, unscaled
  const cls::StatusOr<pluss::Series> predicted = flow.infer(probe);
  std::cout << "ok        = " << std::boolalpha << predicted.ok() << "\n";
  std::cout << "predicted = " << predicted.value().item<double>() << "\n";

  return 0;
}
