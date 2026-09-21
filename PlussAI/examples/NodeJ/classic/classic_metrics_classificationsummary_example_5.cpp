#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // Class 1: tp=2 (rows 7,8), fn=1 (row 9) -> recall = 2/3.
  // Class 1: tn=6 (rows 0-5), fp=1 (row 6) -> specificity = 6/7.
  const pluss::Series truth = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {10}, f64);
  const pluss::Series pred = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 0.0}, {10}, f64);

  const cls::ClassReport default_report = cls::classification_summary(truth, pred).value();
  std::cout << "default (alpha=0.1, squared)   balance_quality = " << default_report.balance_quality << "\n";

  cls::ClassReportOptions no_penalty{};
  no_penalty.balance_quality_alpha = 0.0;
  const cls::ClassReport no_penalty_report = cls::classification_summary(truth, pred, no_penalty).value();
  std::cout << "alpha=0 (no gap penalty)       balance_quality = " << no_penalty_report.balance_quality << "\n";

  cls::ClassReportOptions unsquared{};
  unsquared.square_balance_quality = false;
  const cls::ClassReport unsquared_report = cls::classification_summary(truth, pred, unsquared).value();
  std::cout << "square_balance_quality=false   balance_quality = " << unsquared_report.balance_quality << "\n";

  return 0;
}
