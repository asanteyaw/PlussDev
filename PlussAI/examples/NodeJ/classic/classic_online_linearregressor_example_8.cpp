#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64).reshape({5, 1});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0, 10.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::online::OnlineLinearOptions decaying{};
  decaying.family.objective = cls::LinearObjective::LeastSquares;
  decaying.decaying_rate = true;
  cls::online::OnlineLinearRegressor decaying_model(decaying);
  cls::TrainOptions one{};
  one.max_iterations = 1;
  decaying_model.learn(batch, one);
  for (int i = 0; i < 10; ++i) decaying_model.advance(batch);

  cls::online::OnlineLinearOptions constant = decaying;
  constant.decaying_rate = false;
  cls::online::OnlineLinearRegressor constant_model(constant);
  constant_model.learn(batch, one);
  for (int i = 0; i < 10; ++i) constant_model.advance(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({6.0}, {1}, f64).reshape({1, 1});
  std::cout << "decaying_rate=true  infer([6]) = " << decaying_model.infer(probe).value() << "\n";
  std::cout << "decaying_rate=false infer([6]) = " << constant_model.infer(probe).value() << "\n";

  return 0;
}
