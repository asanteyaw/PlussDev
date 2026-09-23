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
  target.growth = cls::BalanceGrowthKind::ReuseRows;
  cls::RandomState random{};
  random.seed = 7;
  const cls::BalancePlan plan = cls::plan_balance(batch, target, random).value();
  std::cout << "uses_replacement = " << std::boolalpha << plan.uses_replacement << "\n";
  std::cout << "uses_synthesis (false -- ReuseRows never synthesizes) = " << plan.uses_synthesis << "\n";
  std::cout << "indices.numel() (12 = 6+6) = " << plan.indices.numel() << "\n";

  return 0;
}
