#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({-5.0, -0.001, 0.001, 5.0}, {4}, f64).reshape({4, 1});

  cls::prep::ThresholdBinarizer binarizer;  // threshold = 0
  std::cout << "map = " << binarizer.learn_map(batch).value() << "\n";

  return 0;
}
