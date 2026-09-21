#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth = pluss::make_series(
      {0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 2.0}, {9}, f64);
  const pluss::Series pred = pluss::make_series(
      {0.0, 0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 2.0, 2.0}, {9}, f64);

  const cls::ClassReport report = cls::classification_summary(truth, pred).value();
  std::cout << "precision (per class) = " << report.precision << "\n";
  std::cout << "recall (per class)    = " << report.recall << "\n";
  std::cout << "macro_average    [p,r,f1] = " << report.macro_average << "\n";
  std::cout << "weighted_average [p,r,f1] = " << report.weighted_average << "\n";
  std::cout << "micro_average    [p,r,f1] = " << report.micro_average << "\n";
  std::cout << "accuracy   = " << report.accuracy << "\n";
  std::cout << "mcc        = " << report.mcc << "\n";
  std::cout << "cohen_kappa = " << report.cohen_kappa << "\n";

  return 0;
}
