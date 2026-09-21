#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::Logistic;
  cls::online::OnlineLinearClassifier model(options);

  const cls::Series x = pluss::make_series(
      {-1.0, -0.8, -0.6, -0.4, 0.4, 0.6, 0.8, 1.0}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series(
      {10.0, 10.0, 10.0, 10.0, 20.0, 20.0, 20.0, 20.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  model.learn(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({-0.5, 0.5}, {2}, f64).reshape({2, 1});
  std::cout << "infer([-0.5, 0.5]) = " << model.infer(probe).value() << "\n";

  return 0;
}
