#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::Series x32 = pluss::make_series(
      {1.0f, 2.0f, 3.0f, 4.0f}, {4},
      pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU)).reshape({1, 4});
  cls::FeatureBatch batch{};
  batch.x = x32;

  const cls::FeatureBatch reduced = cls::take_features(batch, {0, 2}).value();
  std::cout << "source dtype is Float32 = " << std::boolalpha << (batch.x.scalar_type() == pluss::ScalarType::Float32) << "\n";
  std::cout << "result dtype is Float64 = " << (reduced.x.scalar_type() == pluss::ScalarType::Float64) << "\n";

  return 0;
}
