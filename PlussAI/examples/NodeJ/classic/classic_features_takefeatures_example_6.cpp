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

  const std::vector<std::int64_t> empty{};
  std::cout << "empty indices ok = " << std::boolalpha << cls::take_features(batch, empty).ok() << "\n";
  std::cout << "out-of-range index ok = " << cls::take_features(batch, {5}).ok() << "\n";

  return 0;
}
