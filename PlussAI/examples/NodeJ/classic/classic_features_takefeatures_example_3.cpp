#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({1, 3});
  cls::FeatureBatch batch{};
  batch.x = x;

  const cls::FeatureBatch reduced = cls::take_features(batch, {0, 0, 2}).value();
  std::cout << "feature_count() = " << reduced.feature_count() << "\n";
  std::cout << "reduced.x (column 0 duplicated) = " << reduced.x << "\n";

  return 0;
}
