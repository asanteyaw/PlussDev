#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // class 0 starts at 6 rows; one is a boundary pair with class 1 and
  // will be cleaned away, leaving class 0 at 5.
  const pluss::Series x = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0, 4.0, 20.0, 20.1, 40.0}, {8}, f64).reshape({8, 1});
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Equalize;
  target.clean = cls::BalanceCleanKind::BoundaryPairs;
  target.protect_small_classes = false;
  const cls::BalancePlan plan = cls::plan_balance(batch, target).value();
  std::cout << "before.counts (original, pre-cleaning: [6, 2]) = " << plan.before.counts << "\n";
  std::cout << "removed_indices.numel() (the one boundary-pair row) = " << plan.removed_indices.numel() << "\n";
  std::cout << "target_counts (Equalize against the POST-cleaning max, 5 -- not the original 6) = " << plan.target_counts << "\n";

  return 0;
}
