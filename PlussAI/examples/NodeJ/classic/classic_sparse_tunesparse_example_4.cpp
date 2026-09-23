#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      12, 2,
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
      {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1},
      {1.0, 1.2, 1.4, 1.6, 1.8, 2.0, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0}).value();
  const pluss::Series y = pluss::make_series(
      {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
      {12}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 3;
  split.random.seed = 0;

  // objective_metric not among the resolved (default) {Accuracy} set --
  // every trial fails identically, but the call itself still succeeds.
  const cls::SparseTuneReport report =
      cls::tune_sparse_binary({cls::LinearFamilyOptions{}}, x, y, split, {}, {}, cls::MetricKind::RocAuc).value();
  std::cout << "call ok, trial[0].status.ok() = " << std::boolalpha << report.trials[0].status.ok() << "\n";
  std::cout << "trial[0].objective (NaN)      = " << report.trials[0].objective << "\n";
  std::cout << "best_index (-1, nothing valid) = " << report.best_index << "\n";

  return 0;
}
