#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// metrics::regression(truth, pred).report({...}) fills the regression slots of
// a MetricReport -- report.r2, report.mse, report.rmse, report.mae.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({10., 12., 14., 16., 18., 20., 22., 24.}, {8}, f64);
  const pluss::Series predicted =
      pluss::make_series({10.5, 11.5, 14.2, 15.6, 18.4, 19.7, 22.3, 23.6}, {8}, f64);

  const std::vector<lymph::MetricKind> want = {
      lymph::MetricKind::R2, lymph::MetricKind::Mse,
      lymph::MetricKind::Rmse, lymph::MetricKind::Mae};

  const lymph::metrics::MetricReport report =
      lymph::metrics::regression(truth, predicted).report(want);

  std::cout << "r2   = " << report.r2.item<double>() << "\n";
  std::cout << "mse  = " << report.mse.item<double>() << "\n";
  std::cout << "rmse = " << report.rmse.item<double>() << "\n";
  std::cout << "mae  = " << report.mae.item<double>() << "\n";

  return 0;
}
