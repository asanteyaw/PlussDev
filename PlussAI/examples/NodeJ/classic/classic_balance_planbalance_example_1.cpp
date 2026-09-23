#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {8}, f64);

  const cls::BalancePlan plan = cls::plan_balance(labels, cls::BalanceTarget{}).value();
  std::cout << "target_counts (unchanged: [6, 2]) = " << plan.target_counts << "\n";
  std::cout << "indices.numel() (still 8) = " << plan.indices.numel() << "\n";
  std::cout << "uses_replacement = " << std::boolalpha << plan.uses_replacement << "\n";

  return 0;
}
