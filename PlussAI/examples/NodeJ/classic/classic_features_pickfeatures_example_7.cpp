#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>

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

  cls::FeaturePickOptions too_big{};
  too_big.fraction = 150.0;
  std::cout << "fraction=150 ok = " << std::boolalpha << cls::pick_features(batch, too_big).ok() << "\n";

  cls::FeaturePickOptions non_finite{};
  non_finite.fraction = std::numeric_limits<double>::infinity();
  std::cout << "non-finite fraction ok = " << cls::pick_features(batch, non_finite).ok() << "\n";

  cls::FeaturePickOptions zero_top_k{};
  zero_top_k.top_k = 0;
  std::cout << "top_k=0, fraction unset ok = " << cls::pick_features(batch, zero_top_k).ok() << "\n";

  return 0;
}
