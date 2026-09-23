#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 100.0, 200.0}, {8}, f64).reshape({8, 1});
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Equalize;
  cls::RandomState random{};
  random.seed = 29;

  const cls::BalancePlan plan = cls::plan_balance(batch, target, random).value();
  const cls::LabeledBatch via_balance_rows = cls::balance_rows(batch, target, random).value();
  const cls::LabeledBatch via_direct_slice = cls::slice_labeled_batch(batch, plan.indices).value();

  const double diff = (via_balance_rows.features.x - via_direct_slice.features.x)
                           .abs().amax(0, false).amax(0, false).item<double>();
  std::cout << "uses_synthesis = " << std::boolalpha << plan.uses_synthesis << "\n";
  std::cout << "max abs diff (no synthesis -> identical to a direct slice) = " << diff << "\n";

  return 0;
}
