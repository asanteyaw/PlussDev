#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // 9 majority rows (class 0) spread across [-1, 0.6]; 3 minority rows (class 1)
  // overlapping the majority's upper tail -- no perfect linear separator exists.
  const cls::Series x = pluss::make_series(
      {-1.0, -0.8, -0.6, -0.4, -0.2, 0.0, 0.2, 0.4, 0.6, 0.5, 0.55, 0.6}, {12}, f64).reshape({12, 1});
  const cls::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {12}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::Logistic;

  cls::online::OnlineLinearClassifier unbalanced(options);
  unbalanced.learn(batch);
  cls::AssessOptions recall_only{};
  recall_only.metrics = {cls::MetricKind::Recall};
  std::cout << "recall, no balance = " << unbalanced.assess(batch, recall_only).value() << "\n";

  cls::online::OnlineLinearClassifier balanced_model(options);
  cls::TrainOptions balance_opts{};
  balance_opts.balance.kind = cls::BalanceTargetKind::Equalize;
  balanced_model.learn(batch, balance_opts);
  std::cout << "recall, equalized   = " << balanced_model.assess(batch, recall_only).value() << "\n";

  return 0;
}
