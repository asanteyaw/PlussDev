#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 5.0, 0.1, 1.0, 0.2, 9.0, 10.0, 2.0, 10.1, 8.0, 10.2, 3.0},
      {12}, f64).reshape({6, 2});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::FeatureRankOptions absolute{};
  cls::FeatureRankOptions signed_scores{};
  signed_scores.absolute_score = false;

  const cls::Series abs_scores = cls::rank_features(batch, absolute).value().scores;
  const cls::Series raw_scores = cls::rank_features(batch, signed_scores).value().scores;
  const double diff = (abs_scores - raw_scores).abs().amax(0, false).item<double>();
  std::cout << "max abs diff (absolute_score has no effect) = " << diff << "\n";

  return 0;
}
