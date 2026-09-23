#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 1.0, 5.0,
       10.0, 1.01, 5.0,
       20.0, 0.99, 5.0,
       30.0, 1.02, 5.0},
      {12}, f64).reshape({4, 3});
  cls::FeatureBatch batch{};
  batch.x = x;

  const cls::VariancePruneResult result = cls::variance_prune(batch, 0.01).value();
  std::cout << "selected_feature_indices (only column 0 survives) = " << result.selected_feature_indices << "\n";

  return 0;
}
