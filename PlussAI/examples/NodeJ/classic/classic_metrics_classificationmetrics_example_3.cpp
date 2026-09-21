#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth = pluss::make_series({0.0, 1.0}, {2}, f64);
  const pluss::Series pred = pluss::make_series({0.0, 1.0}, {2}, f64);

  const cls::StatusOr<pluss::Series> values =
      cls::classification_metrics(truth, pred, {cls::MetricKind::RocAuc});
  std::cout << "ok = " << std::boolalpha << values.ok() << "\n";
  std::cout << "message = \"" << values.status().message << "\"\n";

  return 0;
}
