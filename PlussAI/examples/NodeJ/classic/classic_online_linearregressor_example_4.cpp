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
  plain.advance(batch);

  cls::online::OnlineLinearRegressor with_opts(options);
  with_opts.learn(batch);
  cls::TrainOptions custom{};
  custom.max_iterations = 500;  // has no effect on advance()
  with_opts.advance(batch, custom);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({6.0}, {1}, f64).reshape({1, 1});
  const double pred_plain = plain.infer(probe).value().item<double>();
  const double pred_with_opts = with_opts.infer(probe).value().item<double>();
  std::cout << "seen_samples() = " << plain.seen_samples() << "\n";
  std::cout << "max abs diff (options ignored) = " << std::abs(pred_plain - pred_with_opts) << "\n";

  return 0;
}
