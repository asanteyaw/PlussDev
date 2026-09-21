#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.2, 1.4, 1.6, 1.8, 2.0, 2.2, 1.9, 2.1, 2.3, 2.5, 2.7}, {12}, f64).reshape({12, 1});
  const cls::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0}, {12}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  std::vector<cls::ModelSpec> candidates{};
  for (double strength : {1e-4, 1e-4, 1.0, 1000.0}) {
    cls::LinearFamilyOptions opts{};
    opts.objective = cls::LinearObjective::Logistic;
    opts.regularization_strength = strength;
    candidates.push_back(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts});
  }

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 3;
  split.random.seed = 0;

  const cls::TuneReport report = cls::sweep(candidates, batch, split).value();
  std::cout << "best_index = " << report.best_index << "\n";
  for (std::size_t i = 0; i < report.trials.size(); ++i) {
    std::cout << "trial " << i << ": objective=" << report.trials[i].objective
              << " rank=" << report.objective_rank[i] << "\n";
  }

  return 0;
}
