#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {8}, f64);

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Counts;
  target.counts = pluss::make_series({4.0, 4.0}, {2}, f64);  // downsample class 0, oversample class 1
  cls::RandomState random{};
  random.seed = 2;
  const cls::BalancePlan plan = cls::plan_balance(labels, target, random).value();
  std::cout << "target_counts = " << plan.target_counts << "\n";
  std::cout << "after.counts = " << plan.after.counts << "\n";

  return 0;
}
