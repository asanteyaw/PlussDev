#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // feature 0 = y exactly; features 1 and 2 are unrelated.
  const cls::Series x = pluss::make_series(
      {1.0, 5.0, 9.0, 2.0, 1.0, 8.0, 3.0, 9.0, 2.0, 4.0, 2.0, 6.0, 5.0, 8.0, 1.0},
      {15}, f64).reshape({5, 3});
  const cls::Series y = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::ScoreRankOptions options{};
  options.top_k = 2;
  const pluss::Series ranked = cls::f_regression_rank(batch, options).value();
  std::cout << "ranked (feature 0 first) = " << ranked << "\n";

  return 0;
}
