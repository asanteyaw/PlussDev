#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64);
  const pluss::Series pred = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64);

  const pluss::Series values = cls::regression_metrics(truth, pred).value();
  std::cout << "[Mse, Rmse, R2] = " << values << "\n";

  return 0;
}
