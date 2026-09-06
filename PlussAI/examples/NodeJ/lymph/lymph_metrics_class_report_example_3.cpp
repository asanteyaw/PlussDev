#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// The balance rows mirror the average rows but for the imbalance-aware trio:
// micro/macro/weighted_balance are each shape [3] ->
// [specificity, geometric_mean, balance_quality].
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // Imbalanced binary: 8 negatives, 4 positives.
  const pluss::Series truth =
      pluss::make_series({0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1}, {12}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1}, {12}, f64);

  const lymph::ClassReport report =
      lymph::metrics::classification::summary(truth, predicted);

  std::cout << "specificity    = " << report.specificity << "\n";
  std::cout << "geometric_mean = " << report.geometric_mean << "\n";
  std::cout << "balance_quality = " << report.balance_quality << "\n";
  std::cout << "macro_balance (spec, gmean, bq) = " << report.macro_balance << "\n";

  return 0;
}
