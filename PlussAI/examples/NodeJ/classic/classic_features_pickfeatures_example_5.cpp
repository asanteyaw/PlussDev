#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // column 0: weakly separating; column 1: strongly separating.
  const cls::Series x = pluss::make_series(
      {4.0, 0.0, 4.5, 0.1, 5.0, 0.2, 6.0, 10.0, 6.5, 10.1, 7.0, 10.2},
      {12}, f64).reshape({6, 2});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::FeaturePickOptions pick_options{};
  pick_options.top_k = 2;
  const cls::FeaturePickResult picked = cls::pick_features(batch, pick_options).value();
  const cls::VariancePruneResult pruned = cls::variance_prune(batch.features).value();

  std::cout << "pick_features selected_feature_indices  (score order:    [1, 0]) = " << picked.selected_feature_indices << "\n";
  std::cout << "variance_prune selected_feature_indices (original order: [0, 1]) = " << pruned.selected_feature_indices << "\n";

  return 0;
}
