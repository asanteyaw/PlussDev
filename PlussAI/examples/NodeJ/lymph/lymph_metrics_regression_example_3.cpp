#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// The raw Series of requested metrics, in order, is `report(...).values` --
// no separate `*_metric_series` spelling.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace regression = lymph::metrics::regression;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({3.0, 5.0, 7.0, 9.0, 11.0, 13.0, 15.0, 17.0}, {8}, f64);
  const pluss::Series predicted =
      pluss::make_series({3.2, 4.7, 7.1, 9.4, 10.6, 13.1, 15.2, 16.7}, {8}, f64);

  const pluss::Series values =
      regression::report(truth, predicted,
                         {lymph::MetricKind::R2, lymph::MetricKind::Rmse, lymph::MetricKind::Mae})
          .values;

  std::cout << "report values (r2, rmse, mae) = " << values << "\n";

  return 0;
}
