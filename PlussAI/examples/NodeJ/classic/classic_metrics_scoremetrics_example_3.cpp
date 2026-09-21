#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series scores = pluss::make_series({0.9, 0.1}, {2}, f64);
  const pluss::Series labels = pluss::make_series({1.0, 0.0}, {2}, f64);

  const cls::StatusOr<pluss::Series> values = cls::score_metrics(scores, labels, {cls::MetricKind::Mse});
  std::cout << "ok = " << std::boolalpha << values.ok() << "\n";
  std::cout << "message = \"" << values.status().message << "\"\n";

  return 0;
}
