#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // feature 0 separates the two classes cleanly; feature 1 is unrelated noise.
  const cls::Series x = pluss::make_series(
      {0.0, 5.0,
       0.1, 1.0,
       0.2, 9.0,
       10.0, 2.0,
       10.1, 8.0,
       10.2, 3.0},
      {12}, f64).reshape({6, 2});
  const cls::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  const cls::FeatureRankReport report = cls::rank_features(batch).value();
  std::cout << "scores = " << report.scores << "\n";
  std::cout << "ranked_feature_indices = " << report.ranked_feature_indices << "\n";

  return 0;
}
