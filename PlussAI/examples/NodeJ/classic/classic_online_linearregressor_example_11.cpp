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

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::LeastSquares;

  cls::online::OnlineLinearRegressor plain(options);
  plain.learn(batch);

  cls::online::OnlineLinearRegressor with_balance(options);
  cls::TrainOptions balance_opts{};
  balance_opts.balance.kind = cls::BalanceTargetKind::Equalize;
  with_balance.learn(batch, balance_opts);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({6.0}, {1}, f64).reshape({1, 1});
  const double pred_plain = plain.infer(probe).value().item<double>();
  const double pred_balanced = with_balance.infer(probe).value().item<double>();
  std::cout << "max abs diff (balance ignored by the regressor) = "
            << std::abs(pred_plain - pred_balanced) << "\n";

  return 0;
}
