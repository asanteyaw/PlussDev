#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  const pluss::Series pred = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);

  const cls::ClassReport report =
      cls::classification_summary(truth, pred, {0.0, 1.0, 2.0}, 1.0).value();
  std::cout << "labels    = " << report.labels << "\n";
  std::cout << "precision = " << report.precision << "\n";

  return 0;
}
