#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64);
  const pluss::Series pred = pluss::make_series({1.0, 3.0, 3.0, 6.0}, {4}, f64);

  const pluss::Series values =
      cls::regression_metrics(truth, pred, {cls::MetricKind::Mae, cls::MetricKind::Mse}).value();
  std::cout << "[Mae, Mse] = " << values << "\n";

  return 0;
}
