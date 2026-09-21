#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series scores = pluss::make_series({0.9, 0.8, 0.3, 0.1}, {4}, f64);
  const pluss::Series labels = pluss::make_series({1.0, 1.0, 0.0, 0.0}, {4}, f64);

  const pluss::Series values =
      cls::score_metrics(scores, labels, {cls::MetricKind::RocAuc, cls::MetricKind::Accuracy}).value();
  std::cout << "[RocAuc, Accuracy] = " << values << "\n";

  return 0;
}
