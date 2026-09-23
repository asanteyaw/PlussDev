#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // rows 2 and 3 sit right next to each other (distance 0.1) but carry
  // different labels, and each is the other's own nearest neighbor --
  // a boundary pair. Every other row is far from everything else.
  const pluss::Series x = pluss::make_series(
      {0.0, 10.0, 20.0, 20.1, 40.0, 50.0}, {6}, f64).reshape({6, 1});
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::BalanceTarget target{};
  target.clean = cls::BalanceCleanKind::BoundaryPairs;
  target.protect_small_classes = false;  // both classes are size 3, nothing to protect
  const cls::BalancePlan plan = cls::plan_balance(batch, target).value();
  std::cout << "uses_cleaning = " << std::boolalpha << plan.uses_cleaning << "\n";
  std::cout << "removed_indices (expect [2, 3]) = " << plan.removed_indices << "\n";

  return 0;
}
