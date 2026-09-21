#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({0.0, 0.0}, {2}, f64).reshape({1, 2});

  cls::prep::VectorNormalizer normalizer;
  std::cout << "all-zero row -> " << normalizer.learn_map(batch).value() << "\n";

  return 0;
}
