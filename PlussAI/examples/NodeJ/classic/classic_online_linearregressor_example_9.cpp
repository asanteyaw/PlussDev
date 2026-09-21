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

  cls::online::OnlineLinearOptions none{};
  none.family.objective = cls::LinearObjective::LeastSquares;
  none.family.regularization = cls::RegularizationKind::None;
  cls::online::OnlineLinearRegressor none_model(none);
  none_model.learn(batch);

  cls::online::OnlineLinearOptions l2 = none;
  l2.family.regularization = cls::RegularizationKind::L2;
  l2.family.regularization_strength = 5.0;
  cls::online::OnlineLinearRegressor l2_model(l2);
  l2_model.learn(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({6.0}, {1}, f64).reshape({1, 1});
  std::cout << "None infer([6]) = " << none_model.infer(probe).value() << "\n";
  std::cout << "L2   infer([6]) = " << l2_model.infer(probe).value() << "\n";

  return 0;
}
