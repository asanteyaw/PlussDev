#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0}, {10}, f64).reshape({10, 1});
  const cls::Series y = pluss::make_series(
      {2.0, 4.0, 6.0, 8.0, 10.0, 12.0, 14.0, 16.0, 18.0, 20.0}, {10}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions low_reg{};
  low_reg.regularization_strength = 1e-4;
  cls::LinearFamilyOptions high_reg{};
  high_reg.regularization_strength = 10.0;

  const std::vector<cls::ModelSpec> candidates = {
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}},
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, low_reg},
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, high_reg},
  };

  const cls::TuneReport report =
      cls::sweep(candidates, batch, {}, {}, {}, cls::MetricKind::R2).value();
  const cls::TuneLedger& ledger = report.ledger;
  std::cout << "objective      = " << ledger.objective << "\n";
  std::cout << "objective_rank = " << ledger.objective_rank << "\n";
  std::cout << "is_success     = " << ledger.is_success << "\n";
  std::cout << "encoded_params = " << ledger.encoded_params << "\n";

  return 0;
}
