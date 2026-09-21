#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series scores = pluss::make_series({0.9, 0.8, 0.8, 0.4, 0.1}, {5}, f64);
  const pluss::Series labels = pluss::make_series({1.0, 1.0, 0.0, 1.0, 0.0}, {5}, f64);

  const double recall = cls::best_threshold(scores, labels, cls::MetricKind::Recall).value();
  const double sensitivity = cls::best_threshold(scores, labels, cls::MetricKind::Sensitivity).value();
  const double default_f1 = cls::best_threshold(scores, labels).value();
  const double mcc_falls_back = cls::best_threshold(scores, labels, cls::MetricKind::Mcc).value();

  std::cout << "Recall == Sensitivity      = " << std::boolalpha << (recall == sensitivity) << "\n";
  std::cout << "Mcc falls back to F1       = " << (mcc_falls_back == default_f1) << "\n";

  return 0;
}
