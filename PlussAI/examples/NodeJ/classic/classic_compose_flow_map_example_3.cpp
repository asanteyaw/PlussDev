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

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});
  batch.y = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64);
  flow.learn(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({20.0}, {1}, f64).reshape({1, 1});
  std::cout << "map (scaled feature, not a prediction) = " << flow.map(probe).value() << "\n";
  std::cout << "infer (the actual prediction)          = " << flow.infer(probe).value() << "\n";

  return 0;
}
