#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 10.0, 20.0, 30.0, 40.0,
       1.0, 11.0, 21.0, 31.0, 41.0,
       2.0, 12.0, 22.0, 32.0, 42.0},
      {15}, f64).reshape({3, 5});
  cls::FeatureBatch batch{};
  batch.x = x;

  const std::vector<std::int64_t> keep = {2, 0};
  const cls::FeatureBatch reduced = cls::take_features(batch, keep).value();
  std::cout << "feature_count() = " << reduced.feature_count() << "\n";
  std::cout << "reduced.x = " << reduced.x << "\n";

  return 0;
}
