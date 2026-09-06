#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// Mcc (Matthews correlation) and CohenKappa are confusion-matrix metrics, so
// metrics::classification::report(truth, pred, ...) computes them straight from hard labels.
// Gini / AveragePrecision / Brier need scores and are not available on this path.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // 4 positives, 4 negatives; TP=3, FN=1, TN=2, FP=2.
  const pluss::Series truth =
      pluss::make_series({1, 1, 1, 1, 0, 0, 0, 0}, {8}, f64);
  const pluss::Series predicted =
      pluss::make_series({1, 1, 1, 0, 0, 0, 1, 1}, {8}, f64);

  const lymph::metrics::MetricReport report =
      lymph::metrics::classification::report(truth, predicted, {lymph::MetricKind::Mcc, lymph::MetricKind::CohenKappa});

  std::cout << "mcc         = " << report.mcc.item<double>() << "\n";
  std::cout << "cohen_kappa = " << report.cohen_kappa.item<double>() << "\n";
  std::cout << "values      = " << report.values << "\n";

  return 0;
}
