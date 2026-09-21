#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const double nan = std::numeric_limits<double>::quiet_NaN();

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({3.0, nan, 4.0}, {3}, f64).reshape({1, 3});  // L2 norm over {3, 4} is 5

  cls::prep::VectorNormalizer normalizer;  // L2 by default
  std::cout << "map = " << normalizer.learn_map(batch).value() << "\n";

  return 0;
}
