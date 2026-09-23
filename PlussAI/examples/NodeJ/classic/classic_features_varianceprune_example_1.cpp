#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // column 0: varies a lot; column 1: varies a little; column 2: constant.
  const cls::Series x = pluss::make_series(
      {0.0, 1.0, 5.0,
       10.0, 1.01, 5.0,
       20.0, 0.99, 5.0,
       30.0, 1.02, 5.0},
      {12}, f64).reshape({4, 3});
  cls::FeatureBatch batch{};
  batch.x = x;

  const cls::VariancePruneResult result = cls::variance_prune(batch).value();
  std::cout << "feature_variances = " << result.feature_variances << "\n";
  std::cout << "selected_feature_indices = " << result.selected_feature_indices << "\n";
  std::cout << "reduced.feature_count() = " << result.reduced.feature_count() << "\n";

  return 0;
}
