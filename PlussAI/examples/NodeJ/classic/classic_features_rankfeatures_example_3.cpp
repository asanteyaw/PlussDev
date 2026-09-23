#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // feature 0 = y exactly (perfect correlation); feature 1 is unrelated.
  const cls::Series x = pluss::make_series(
      {1.0, 5.0, 2.0, 1.0, 3.0, 9.0, 4.0, 2.0, 5.0, 8.0}, {10}, f64).reshape({5, 2});
  const cls::Series y = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::FeatureRankOptions options{};
  options.mode = cls::FeatureScoreMode::RegressionF;
  const cls::FeatureRankReport report = cls::rank_features(batch, options).value();
  std::cout << "scores = " << report.scores << "\n";
  std::cout << "ranked_feature_indices (feature 0 first) = " << report.ranked_feature_indices << "\n";

  return 0;
}
