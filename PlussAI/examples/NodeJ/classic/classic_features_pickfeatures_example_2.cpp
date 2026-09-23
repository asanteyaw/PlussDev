#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {5.0, 0.0, 9.0, 0.0, 1.0, 0.1, 3.0, 0.2, 9.0, 0.2, 1.0, 0.1,
       2.0, 10.0, 5.0, 10.0, 8.0, 10.1, 2.0, 10.2, 3.0, 10.2, 8.0, 10.1},
      {24}, f64).reshape({6, 4});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::FeaturePickOptions options{};
  options.fraction = 0.5;
  const cls::FeaturePickResult result = cls::pick_features(batch, options).value();
  std::cout << "reduced.feature_count() (ceil(0.5*4) = 2) = " << result.reduced.feature_count() << "\n";

  return 0;
}
