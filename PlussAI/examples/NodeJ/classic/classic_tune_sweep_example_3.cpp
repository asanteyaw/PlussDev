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

  const std::vector<cls::ModelSpec> candidates = {
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}},
  };

  // objective_metric left at its default: MetricKind::Accuracy.
  const cls::TuneReport report = cls::sweep(candidates, batch).value();
  std::cout << "best_index = " << report.best_index << "\n";
  std::cout << "trial 0 ok = " << std::boolalpha << report.trials[0].status.ok() << "\n";
  std::cout << "trial 0 message = \"" << report.trials[0].status.message << "\"\n";

  return 0;
}
