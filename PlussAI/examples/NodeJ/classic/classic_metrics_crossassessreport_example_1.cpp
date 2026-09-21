#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.1, 1.2, 1.3, 2.0, 2.1, 2.2, 2.3, 3.0, 3.1, 3.2, 3.3}, {12}, f64).reshape({12, 1});
  const cls::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0}, {12}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 3;
  split.random.seed = 0;

  const cls::CrossAssessReport report =
      cls::cross_assess(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear},
                        batch, split).value();

  std::cout << "fold count = " << report.folds.size() << "\n";
  for (std::size_t i = 0; i < report.folds.size(); ++i) {
    std::cout << "fold " << i << " accuracy = " << report.folds[i].metrics << "\n";
  }
  std::cout << "mean = " << report.mean_metrics << "\n";
  std::cout << "std  = " << report.std_metrics << "\n";

  return 0;
}
