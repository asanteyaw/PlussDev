#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::LeastSquares;
  cls::online::OnlineLinearRegressor model(options);
  std::cout << "trained() before learn = " << std::boolalpha << model.trained() << "\n";

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64).reshape({5, 1});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0, 10.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  const cls::Status learned = model.learn(batch);
  std::cout << "learn ok = " << learned.ok() << "\n";
  std::cout << "trained() after learn = " << model.trained() << "\n";

  model.advance(batch);
  std::cout << "seen_samples() after learn + 1 advance = " << model.seen_samples() << "\n";

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({6.0}, {1}, f64).reshape({1, 1});
  std::cout << "infer([6]) = " << model.infer(probe).value() << "\n";
  std::cout << "assess (default [Mse, Rmse, R2]) = " << model.assess(batch).value() << "\n";

  return 0;
}
