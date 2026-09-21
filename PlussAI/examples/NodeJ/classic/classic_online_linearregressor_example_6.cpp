#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // y = 2x for every row except the last, which is a wild outlier.
  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64).reshape({5, 1});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0, 100.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::online::OnlineLinearOptions least_squares{};
  least_squares.family.objective = cls::LinearObjective::LeastSquares;
  cls::online::OnlineLinearRegressor ls_model(least_squares);
  ls_model.learn(batch);

  cls::online::OnlineLinearOptions robust{};
  robust.family.objective = cls::LinearObjective::RobustLeast;
  robust.family.robust_transition = 1.0;
  cls::online::OnlineLinearRegressor robust_model(robust);
  robust_model.learn(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({6.0}, {1}, f64).reshape({1, 1});
  std::cout << "LeastSquares infer([6]) (pulled toward the outlier) = " << ls_model.infer(probe).value() << "\n";
  std::cout << "RobustLeast  infer([6]) (closer to the true trend)  = " << robust_model.infer(probe).value() << "\n";

  return 0;
}
