#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // class 1 has only 1 row, sitting right at a boundary with class 0.
  const pluss::Series x = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0, 4.0, 4.1}, {6}, f64).reshape({6, 1});
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::BalanceTarget protected_target{};
  protected_target.clean = cls::BalanceCleanKind::BoundaryPairs;
  // protect_small_classes defaults to true.
  const cls::StatusOr<cls::BalancePlan> protected_plan = cls::plan_balance(batch, protected_target);
  std::cout << "protect_small_classes=true  ok = " << std::boolalpha << protected_plan.ok() << "\n";
  if (protected_plan.ok()) {
    std::cout << "  removed_indices (row 4, class 0's side of the pair -- row 5 survives) = "
              << protected_plan.value().removed_indices << "\n";
  }

  cls::BalanceTarget unprotected_target = protected_target;
  unprotected_target.protect_small_classes = false;
  const cls::StatusOr<cls::BalancePlan> unprotected_plan = cls::plan_balance(batch, unprotected_target);
  std::cout << "protect_small_classes=false ok = " << unprotected_plan.ok() << "\n";
  if (!unprotected_plan.ok()) {
    std::cout << "  message = \"" << unprotected_plan.status().message << "\"\n";
  }

  return 0;
}
