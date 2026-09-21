#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0}, {12}, f64).reshape({12, 1});
  const cls::Series y = pluss::make_series(
      {2.0, 4.0, 6.0, 8.0, 10.0, 12.0, 14.0, 16.0, 18.0, 20.0, 22.0, 24.0}, {12}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::TSCrossVal;
  split.folds = 3;
  split.shuffle = false;

  const cls::CrossAssessReport report = cls::cross_assess(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}},
      batch, split).value();
  std::cout << "fold count = " << report.folds.size() << "\n";
  for (std::size_t i = 0; i < report.folds.size(); ++i) {
    std::cout << "fold " << i << " test_indices = " << report.folds[i].test_indices << "\n";
  }

  cls::SplitOptions shuffled = split;
  shuffled.shuffle = true;
  const cls::StatusOr<cls::CrossAssessReport> rejected = cls::cross_assess(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}},
      batch, shuffled);
  std::cout << "shuffle=true ok = " << std::boolalpha << rejected.ok() << "\n";

  return 0;
}
