#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // features 0 and 1 are identical copies of each other -- same F-score.
  const cls::Series x = pluss::make_series(
      {0.0, 0.0, 1.0, 1.0, 2.0, 2.0, 10.0, 10.0, 11.0, 11.0, 12.0, 12.0},
      {12}, f64).reshape({6, 2});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  const cls::FeatureRankReport report = cls::rank_features(batch).value();
  std::cout << "scores (tied) = " << report.scores << "\n";
  std::cout << "ranked_feature_indices (tie -> ascending index: [0, 1]) = " << report.ranked_feature_indices << "\n";

  return 0;
}
