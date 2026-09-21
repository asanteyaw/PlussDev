#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({3.0, 4.0}, {2}, f64).reshape({1, 2});  // L2 norm is exactly 5

  auto normalize = [&](cls::prep::VectorNorm norm) {
    cls::prep::VectorNormalizerOptions options{};
    options.norm = norm;
    cls::prep::VectorNormalizer normalizer(options);
    return normalizer.learn_map(batch).value();
  };

  std::cout << "L1:   " << normalize(cls::prep::VectorNorm::L1) << "\n";
  std::cout << "L2:   " << normalize(cls::prep::VectorNorm::L2) << "\n";
  std::cout << "Linf: " << normalize(cls::prep::VectorNorm::Linf) << "\n";

  return 0;
}
