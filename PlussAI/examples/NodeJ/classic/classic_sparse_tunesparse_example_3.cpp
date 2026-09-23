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

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::StratifiedCrossVal;
  split.folds = 3;
  split.random.seed = 0;

  cls::LinearFamilyOptions none{};
  cls::LinearFamilyOptions l2{};
  l2.regularization = cls::RegularizationKind::L2;
  l2.regularization_strength = 1.0;

  const cls::SparseTuneReport auto_policy =
      cls::tune_sparse_multiclass({none, l2}, x, y, split, {}, {}, cls::MetricKind::Accuracy).value();
  const cls::SparseTuneReport forced_max =
      cls::tune_sparse_multiclass({none, l2}, x, y, split, {}, {}, cls::MetricKind::Accuracy,
                                  cls::SparseObjectivePolicy::Maximize)
          .value();
  std::cout << "best_index matches = " << std::boolalpha << (auto_policy.best_index == forced_max.best_index) << "\n";

  return 0;
}
