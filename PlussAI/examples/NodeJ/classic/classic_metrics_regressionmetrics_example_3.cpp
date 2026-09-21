#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth = pluss::make_series({5.0, 5.0, 5.0}, {3}, f64);
  const pluss::Series pred = pluss::make_series({4.0, 5.0, 6.0}, {3}, f64);

  const pluss::Series values = cls::regression_metrics(truth, pred, {cls::MetricKind::R2}).value();
  std::cout << "R2 (constant truth) = " << values << "\n";

  return 0;
}
