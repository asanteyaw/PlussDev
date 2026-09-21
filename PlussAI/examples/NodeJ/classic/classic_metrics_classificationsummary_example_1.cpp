#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // 9 majority-class rows, all correct; 1 minority-class row, predicted wrong.
  const pluss::Series truth = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0}, {10}, f64);
  const pluss::Series pred = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}, {10}, f64);

  const cls::ClassReport report = cls::classification_summary(truth, pred).value();
  std::cout << "accuracy   = " << report.accuracy << "\n";
  std::cout << "recall (per class)       = " << report.recall << "\n";
  std::cout << "specificity (per class)  = " << report.specificity << "\n";
  std::cout << "balance_quality (per class) = " << report.balance_quality << "\n";

  return 0;
}
