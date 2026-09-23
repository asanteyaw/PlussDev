#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 5.0, 2.0, 1.0, 3.0, 9.0, 4.0, 2.0, 5.0, 8.0}, {10}, f64).reshape({5, 2});
  const cls::Series y = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::FeaturePickOptions options{};
  options.mode = cls::FeatureScoreMode::RegressionF;
  options.top_k = 1;
  const cls::FeaturePickResult result = cls::pick_features(batch, options).value();
  std::cout << "selected_feature_indices (column 0, the correlated one) = " << result.selected_feature_indices << "\n";

  return 0;
}
