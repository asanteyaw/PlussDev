#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.9, 2.0, 2.1, 2.2}, {12}, f64).reshape({12, 1});
  const cls::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0}, {12}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions logistic_a{};
  logistic_a.objective = cls::LinearObjective::Logistic;
  cls::LinearFamilyOptions logistic_b{};
  logistic_b.objective = cls::LinearObjective::Logistic;
  logistic_b.regularization_strength = 0.5;
  // Default `objective = LeastSquares` builds fine but fails learn() for a
  // classification task -- see LinearFamilyOptions.md#objective.
  cls::LinearFamilyOptions broken{};

  const std::vector<cls::ModelSpec> candidates = {
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, logistic_a},
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, logistic_b},
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, broken},
  };

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 3;
  split.random.seed = 0;

  const cls::TuneReport report =
      cls::sweep(candidates, batch, split, {}, {}, cls::MetricKind::Accuracy).value();
  std::cout << "best_index = " << report.best_index << "\n";
  for (std::size_t i = 0; i < report.trials.size(); ++i) {
    std::cout << "trial " << i << ": ok=" << std::boolalpha << report.trials[i].status.ok()
              << " objective=" << report.trials[i].objective
              << " rank=" << report.objective_rank[i] << "\n";
  }

  return 0;
}
