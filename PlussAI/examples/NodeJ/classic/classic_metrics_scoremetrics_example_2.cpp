#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // 4 rows, 3 classes -- each row's own argmax matches its true label.
  const pluss::Series scores = pluss::make_series(
      {0.7, 0.2, 0.1,
       0.1, 0.8, 0.1,
       0.2, 0.2, 0.6,
       0.6, 0.3, 0.1}, {12}, f64).reshape({4, 3});
  const pluss::Series labels = pluss::make_series({0.0, 1.0, 2.0, 0.0}, {4}, f64);

  const pluss::Series values =
      cls::score_metrics(scores, labels, {cls::MetricKind::LogLoss, cls::MetricKind::Accuracy}).value();
  std::cout << "[LogLoss, Accuracy] = " << values << "\n";

  return 0;
}
