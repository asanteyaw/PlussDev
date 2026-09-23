#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {5.0, 0.0, 1.0, 1.0, 0.1, 2.0, 9.0, 0.2, 3.0,
       2.0, 10.0, 4.0, 8.0, 10.1, 5.0, 3.0, 10.2, 6.0},
      {18}, f64).reshape({6, 3});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::ScoreRankOptions absolute{};
  cls::ScoreRankOptions signed_scores{};
  signed_scores.absolute_score = false;

  const pluss::Series a = cls::f_score_rank(batch, absolute).value();
  const pluss::Series b = cls::f_score_rank(batch, signed_scores).value();
  const double diff = (a.to(pluss::ScalarType::Float64) - b.to(pluss::ScalarType::Float64))
                           .abs().amax(0, false).item<double>();
  std::cout << "max abs diff (absolute_score has no effect) = " << diff << "\n";

  return 0;
}
