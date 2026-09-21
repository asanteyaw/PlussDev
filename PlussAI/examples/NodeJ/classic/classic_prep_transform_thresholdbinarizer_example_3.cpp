#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::ThresholdBinarizerOptions options{};
  options.threshold = 0.0;
  options.low_value = -1.0;
  options.high_value = 1.0;
  cls::prep::ThresholdBinarizer binarizer(options);

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({-2.0, 2.0}, {2}, f64).reshape({2, 1});
  std::cout << "map([-2, 2]) = " << binarizer.learn_map(batch).value() << "\n";

  return 0;
}
