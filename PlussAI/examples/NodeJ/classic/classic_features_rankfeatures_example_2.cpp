#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 5.0, 0.1, 5.1, 5.0, 5.2, 5.1, 4.9, 10.0, 5.0, 10.1, 5.1},
      {12}, f64).reshape({6, 2});
  const cls::Series y = pluss::make_series(
      {0.0, 0.0, 1.0, 1.0, 2.0, 2.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  const cls::FeatureRankReport report = cls::rank_features(batch).value();
  std::cout << "scores (feature 0 separates 3 classes, feature 1 does not) = " << report.scores << "\n";

  return 0;
}
