#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  const pluss::Series pred = pluss::make_series({0.0, 1.0, 1.0, 1.0}, {4}, f64);

  const cls::ClassReport unweighted = cls::classification_summary(truth, pred).value();
  std::cout << "unweighted recall = " << unweighted.recall << "\n";

  cls::ClassReportOptions options{};
  options.sample_weights = pluss::make_series({1.0, 0.0, 1.0, 1.0}, {4}, f64);
  const cls::ClassReport weighted = cls::classification_summary(truth, pred, options).value();
  std::cout << "weighted recall   = " << weighted.recall << "\n";

  return 0;
}
