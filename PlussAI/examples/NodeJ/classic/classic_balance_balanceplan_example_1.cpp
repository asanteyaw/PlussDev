#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

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
  cls::RandomState random{};
  random.seed = 4;
  const cls::BalancePlan plan = cls::plan_balance(batch, target, random).value();

  std::cout << "before.counts = " << plan.before.counts << "\n";
  std::cout << "after.counts  = " << plan.after.counts << "\n";
  std::cout << "uses_cleaning = " << std::boolalpha << plan.uses_cleaning << "\n";
  std::cout << "uses_replacement = " << plan.uses_replacement << "\n";
  std::cout << "uses_synthesis = " << plan.uses_synthesis << "\n";
  std::cout << "removed_indices.numel() = " << plan.removed_indices.numel() << "\n";

  return 0;
}
