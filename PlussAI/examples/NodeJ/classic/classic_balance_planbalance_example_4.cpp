#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // class 0: 10 rows, class 1: 2 rows -- current ratio 0.2.
  std::vector<double> raw(10, 0.0);
  raw.push_back(1.0);
  raw.push_back(1.0);
  const pluss::Series labels = pluss::make_series(raw, {static_cast<std::int64_t>(raw.size())}, f64);

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Ratio;
  target.ratio = 0.5;  // desired minority >= ceil(0.5 * 10) = 5
  cls::RandomState random{};
  random.seed = 3;
  const cls::BalancePlan plan = cls::plan_balance(labels, target, random).value();
  std::cout << "target_counts (majority unchanged=10, minority grows to 5) = " << plan.target_counts << "\n";

  return 0;
}
