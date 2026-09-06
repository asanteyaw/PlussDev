#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// classification::report({...}) wraps the chosen metrics in a MetricReport;
// report(...) with no metric list uses the classification default set, and
// `.values` is the raw Series behind it.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace classification = lymph::metrics::classification;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 1, 0, 1, 1, 0, 1, 1}, {10}, f64);

  const lymph::metrics::MetricReport report =
      classification::report(truth, predicted,
                             {lymph::MetricKind::Accuracy, lymph::MetricKind::Precision,
                              lymph::MetricKind::Recall, lymph::MetricKind::F1});
  std::cout << "report.accuracy  = " << report.accuracy.item<double>() << "\n";
  std::cout << "report.precision = " << report.precision.item<double>() << "\n";
  std::cout << "report.recall    = " << report.recall.item<double>() << "\n";
  std::cout << "report.f1        = " << report.f1.item<double>() << "\n";

  const pluss::Series defaults = classification::report(truth, predicted).values;
  std::cout << "default values (" << defaults.numel() << " metrics) = " << defaults << "\n";

  return 0;
}
