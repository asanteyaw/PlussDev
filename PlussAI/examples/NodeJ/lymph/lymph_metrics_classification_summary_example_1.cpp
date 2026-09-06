#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// classification_summary(truth, pred) computes a ClassReport from two label
// Series: per-class precision / recall / f1 / support, plus the accuracy and
// the macro / weighted averages.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 1, 0, 1, 1, 0, 1, 1}, {10}, f64);

  const lymph::ClassReport report =
      lymph::metrics::classification::summary(truth, predicted);

  std::cout << "labels    = " << report.labels << "\n";
  std::cout << "support   = " << report.support << "\n";
  std::cout << "precision = " << report.precision << "\n";
  std::cout << "recall    = " << report.recall << "\n";
  std::cout << "f1        = " << report.f1 << "\n";
  std::cout << "accuracy  = " << report.accuracy.item<double>() << "\n";
  std::cout << "macro_average (p,r,f1) = " << report.macro_average << "\n";

  return 0;
}
