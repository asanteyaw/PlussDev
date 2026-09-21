#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series scores = pluss::make_series({0.9, 0.8, 0.8, 0.4, 0.1}, {5}, f64);
  const pluss::Series labels = pluss::make_series({1.0, 1.0, 0.0, 1.0, 0.0}, {5}, f64);

  const cls::ThresholdCurve curve = cls::threshold_curve(scores, labels).value();
  std::cout << "thresholds = " << curve.thresholds << "\n";
  std::cout << "precision  = " << curve.precision << "\n";
  std::cout << "recall     = " << curve.recall << "\n";
  std::cout << "f1         = " << curve.f1 << "\n";

  return 0;
}
