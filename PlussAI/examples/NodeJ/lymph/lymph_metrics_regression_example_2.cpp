#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// regression::report() with no metric list uses the regression default set
// (mse / rmse / r2). requested reports which ones came back.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({3.0, 5.0, 7.0, 9.0, 11.0, 13.0, 15.0, 17.0}, {8}, f64);
  const pluss::Series predicted =
      pluss::make_series({3.2, 4.7, 7.1, 9.4, 10.6, 13.1, 15.2, 16.7}, {8}, f64);

  const lymph::metrics::MetricReport report =
      lymph::metrics::regression::report(truth, predicted);

  const std::vector<double> values = report.values.to_vec<double>();
  std::cout << "default regression metrics (" << report.requested.size() << "):\n";
  for (std::size_t i = 0; i < report.requested.size(); ++i) {
    std::cout << "  kind " << static_cast<int>(report.requested[i])
              << " = " << values[i] << "\n";
  }

  return 0;
}
