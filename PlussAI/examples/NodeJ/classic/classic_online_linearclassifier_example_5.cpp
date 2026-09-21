#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::Logistic;

  const cls::Series x = pluss::make_series(
      {-1.0, -0.8, -0.6, -0.4, 0.4, 0.6, 0.8, 1.0}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::online::OnlineLinearClassifier plain(options);
  plain.learn(batch);
  plain.advance(batch);
  std::cout << "seen_samples() after learn + 1 advance = " << plain.seen_samples() << "\n";

  cls::online::OnlineLinearClassifier with_balance_opt(options);
  with_balance_opt.learn(batch);
  cls::TrainOptions balance_opts{};
  balance_opts.balance.kind = cls::BalanceTargetKind::Equalize;
  with_balance_opt.advance(batch, balance_opts);  // balance is ignored by advance()

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({0.7}, {1}, f64).reshape({1, 1});
  const double score_plain = plain.infer_scores(probe).value().item<double>();
  const double score_with_opts = with_balance_opt.infer_scores(probe).value().item<double>();
  std::cout << "max abs diff (options ignored) = " << std::abs(score_plain - score_with_opts) << "\n";

  return 0;
}
