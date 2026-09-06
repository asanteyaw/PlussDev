#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// metrics::regression::report(truth, pred, {...}) is the regression twin of
// metrics::classification::report: a MetricReport whose `.values` is the raw
// Series of the requested metrics in order.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace regression = lymph::metrics::regression;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({10., 12., 14., 16., 18., 20., 22., 24.}, {8}, f64);
  const pluss::Series predicted =
      pluss::make_series({10.5, 11.5, 14.2, 15.6, 18.4, 19.7, 22.3, 23.6}, {8}, f64);

  const lymph::metrics::MetricReport report =
      regression::report(truth, predicted, {lymph::MetricKind::R2, lymph::MetricKind::Mse});

  std::cout << "values (r2, mse) = " << report.values << "\n";
  std::cout << "report.r2  = " << report.r2.item<double>() << "\n";
  std::cout << "report.mse = " << report.mse.item<double>() << "\n";

  return 0;
}
