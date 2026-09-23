#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // Column 0's population variance ({-1, 1}) is exactly 1.0: mean=0,
  // sum((x-0)^2)/2 = 1.0. Column 1 always has a clearly higher variance,
  // so it survives either floor and the never-empty fallback never has a
  // chance to mask column 0's own drop/keep outcome.
  const cls::Series x = pluss::make_series(
      {-1.0, -10.0, 1.0, 10.0}, {4}, f64).reshape({2, 2});
  cls::FeatureBatch batch{};
  batch.x = x;

  const cls::VariancePruneResult at_floor = cls::variance_prune(batch, 1.0).value();
  const cls::VariancePruneResult below_floor = cls::variance_prune(batch, 0.999999).value();
  std::cout << "floor == column 0's variance exactly -> selected_feature_indices = " << at_floor.selected_feature_indices << "\n";
  std::cout << "floor slightly below                 -> selected_feature_indices = " << below_floor.selected_feature_indices << "\n";

  return 0;
}
