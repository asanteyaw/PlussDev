#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 5.0, 9.0, 2.0, 1.0, 8.0, 3.0, 9.0, 2.0, 4.0, 2.0, 6.0, 5.0, 8.0, 1.0},
      {15}, f64).reshape({5, 3});
  const cls::Series y = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::ScoreRankOptions score_options{};
  score_options.top_k = 2;
  const pluss::Series via_f_regression_rank = cls::f_regression_rank(batch, score_options).value();

  cls::FeaturePickOptions pick_options{};
  pick_options.mode = cls::FeatureScoreMode::RegressionF;
  pick_options.top_k = 2;
  const pluss::Series via_pick_features = cls::pick_features(batch, pick_options).value().selected_feature_indices;

  const double diff = (via_f_regression_rank.to(pluss::ScalarType::Float64)
                        - via_pick_features.to(pluss::ScalarType::Float64))
                           .abs().amax(0, false).item<double>();
  std::cout << "max abs diff (identical index lists) = " << diff << "\n";

  return 0;
}
