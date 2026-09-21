#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.05, 2.0, 2.05, 3.0, 3.05, 4.0, 4.05}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series(
      {2.0, 2.1, 4.0, 4.1, 6.0, 6.1, 8.0, 8.1}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::GroupCrossVal;
  split.groups = pluss::make_series({0.0, 0.0, 1.0, 1.0, 2.0, 2.0, 3.0, 3.0}, {8}, f64);
  split.folds = 2;

  const cls::CrossAssessReport report = cls::cross_assess(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}},
      batch, split).value();
  std::cout << "fold count = " << report.folds.size() << "\n";
  for (std::size_t i = 0; i < report.folds.size(); ++i) {
    std::cout << "fold " << i << " test_indices = " << report.folds[i].test_indices << "\n";
  }

  return 0;
}
