#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      9, 3,
      {0, 1, 2, 3, 4, 5, 6, 7, 8},
      {0, 0, 0, 1, 1, 1, 2, 2, 2},
      {1.0, 2.0, 3.0, 1.0, 2.0, 3.0, 1.0, 2.0, 3.0}).value();
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 2.0},
      {9}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  std::vector<cls::LinearFamilyOptions> candidates{};
  candidates.push_back(cls::LinearFamilyOptions{});
  cls::LinearFamilyOptions l2{};
  l2.regularization = cls::RegularizationKind::L2;
  l2.regularization_strength = 1.0;
  candidates.push_back(l2);

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::StratifiedCrossVal;
  split.folds = 3;
  split.random.seed = 0;

  cls::AssessOptions balanced{};
  balanced.metrics = {cls::MetricKind::BalancedAccuracy};

  const cls::SparseTuneReport report =
      cls::sweep_sparse_multiclass(candidates, x, y, split, {}, balanced, cls::MetricKind::BalancedAccuracy).value();
  std::cout << "best_index = " << report.best_index << "\n";

  return 0;
}
