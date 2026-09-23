#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const std::vector<std::int64_t> rows_v = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  const cls::SparseTripletMatrix x = cls::make_sparse_triplet(
      10, 1, rows_v, std::vector<std::int64_t>(10, 0),
      {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0}).value();
  const pluss::Series y = pluss::make_series(
      {0.2, 0.4, 0.6, 0.8, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0},
      {10}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  std::vector<cls::LinearFamilyOptions> candidates{};
  candidates.push_back(cls::LinearFamilyOptions{});
  cls::LinearFamilyOptions l2{};
  l2.regularization = cls::RegularizationKind::L2;
  l2.regularization_strength = 0.5;
  candidates.push_back(l2);

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 5;
  split.random.seed = 0;

  const cls::SparseTuneReport report =
      cls::sweep_sparse_regression(candidates, x, y, split, {}, {}, cls::MetricKind::Mse).value();
  std::cout << "best_index = " << report.best_index << "\n";

  return 0;
}
