#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::LinearFamilyOptions linear{};
  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, linear});
  cls::Flow flow = made.move_value();

  const cls::Series x = pluss::make_series(
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0}, {10}, f64).reshape({10, 1});
  const cls::Series y = pluss::make_series(
      {10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0}, {10}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({10.0}, {1}, f64).reshape({1, 1});

  auto fit_and_predict = [&](double strength) {
    flow.set_nested_param("model::opt.regularization_strength", strength);
    flow.learn(batch);
    return flow.infer(probe).value().item<double>();
  };

  std::cout << "strength=1e-6 prediction : " << fit_and_predict(1e-6) << "\n";
  std::cout << "strength=1e3  prediction : " << fit_and_predict(1e3) << "\n";

  return 0;
}
