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

  std::vector<cls::LinearFamilyOptions> candidates{};
  for (double strength : {1e-4, 1e-2, 1.0, 100.0}) {
    cls::LinearFamilyOptions opts{};
    opts.regularization = cls::RegularizationKind::L2;
    opts.regularization_strength = strength;
    candidates.push_back(opts);
  }

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 3;
  split.random.seed = 0;

  const cls::SparseTuneReport report = cls::tune_sparse_binary(candidates, x, y, split).value();
  std::cout << "best_index = " << report.best_index << "\n";
  for (std::size_t i = 0; i < report.trials.size(); ++i) {
    std::cout << "trial " << i << ": objective=" << report.trials[i].objective
              << " rank=" << report.objective_rank[i] << "\n";
  }
  std::cout << "ledger.encoded_param_feature_names[2] = " << report.ledger.encoded_param_feature_names[2] << "\n";
  std::cout << "ledger.encoded_params = " << report.ledger.encoded_params << "\n";

  return 0;
}
