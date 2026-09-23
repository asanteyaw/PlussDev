#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // Both columns are constant; column 1 (value 5.0) has zero variance,
  // column 0 (value 1.0) also has zero variance -- but the fallback still
  // keeps exactly one.
  const cls::Series x = pluss::make_series(
      {1.0, 5.0, 1.0, 5.0}, {4}, f64).reshape({2, 2});
  cls::FeatureBatch batch{};
  batch.x = x;

  const cls::VariancePruneResult result = cls::variance_prune(batch, 0.5).value();
  std::cout << "feature_variances = " << result.feature_variances << "\n";
  std::cout << "selected_feature_indices (never empty) = " << result.selected_feature_indices << "\n";
  std::cout << "reduced.feature_count() = " << result.reduced.feature_count() << "\n";

  return 0;
}
