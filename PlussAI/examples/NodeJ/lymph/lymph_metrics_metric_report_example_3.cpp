#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// Calling report() with no argument uses the task's default metric set. The
// returned MetricReport still exposes `requested` (the kinds, in order) aligned
// with `values`.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1, 1, 1}, {10}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 1, 1, 1, 1, 1, 0, 1}, {10}, f64);

  const lymph::metrics::MetricReport report =
      lymph::metrics::classification::report(truth, predicted);

  const std::vector<double> values = report.values.host<double>();
  std::cout << "default classification metrics (" << report.requested.size() << "):\n";
  for (std::size_t i = 0; i < report.requested.size(); ++i) {
    std::cout << "  kind " << static_cast<int>(report.requested[i])
              << " = " << values[i] << "\n";
  }
  std::cout << "balanced_accuracy defined? " << std::boolalpha
            << report.balanced_accuracy.defined() << "\n";
  std::cout << "roc_auc defined? " << report.roc_auc.defined()
            << " (not in the default set)\n";

  return 0;
}
