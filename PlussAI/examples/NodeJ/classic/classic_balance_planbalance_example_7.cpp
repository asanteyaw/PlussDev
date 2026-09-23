#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // class 0: 5 rows spread out; class 1: exactly 1 row (too few to blend).
  const pluss::Series x = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0, 4.0, 100.0}, {6}, f64).reshape({6, 1});
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Counts;
  target.counts = pluss::make_series({8.0, 3.0}, {2}, f64);
  target.growth = cls::BalanceGrowthKind::BlendNeighbors;
  target.neighbors = 2;
  cls::RandomState random{};
  random.seed = 11;
  const cls::BalancePlan plan = cls::plan_balance(batch, target, random).value();

  std::cout << "uses_synthesis = " << std::boolalpha << plan.uses_synthesis << "\n";
  std::cout << "uses_replacement (class 1's fallback) = " << plan.uses_replacement << "\n";
  std::cout << "synthetic_labels = " << plan.synthetic_labels << "\n";
  std::cout << "synthetic_steps (each in [0,1)) = " << plan.synthetic_steps << "\n";

  return 0;
}
