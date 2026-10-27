#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// metrics::classification::report(truth, pred, {...}) returns a MetricReport:
// a `values` Series aligned to `requested`, plus a named 1-element Series per
// requested metric (report.accuracy, report.f1, ...).
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1, 1, 1}, {10}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 1, 1, 1, 1, 1, 0, 1}, {10}, f64);

  const std::vector<lymph::MetricKind> want = {
      lymph::MetricKind::Accuracy, lymph::MetricKind::Precision,
      lymph::MetricKind::Recall, lymph::MetricKind::F1};

  const lymph::metrics::MetricReport report =
      lymph::metrics::classification::report(truth, predicted, want);

  const std::vector<double> values = report.values.host<double>();
  std::cout << "requested x values:\n";
  for (std::size_t i = 0; i < report.requested.size(); ++i) {
    std::cout << "  metric[" << i << "] = " << values[i] << "\n";
  }
  std::cout << "report.accuracy = " << report.accuracy.item<double>() << "\n";
  std::cout << "report.f1       = " << report.f1.item<double>() << "\n";

  return 0;
}
