#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::ThresholdBinarizerOptions options{};
  options.threshold = 10.0;
  cls::prep::ThresholdBinarizer binarizer(options);

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({9.999, 10.0, 10.001}, {3}, f64).reshape({3, 1});
  std::cout << "map([9.999, 10.0 (== threshold), 10.001]) = " << binarizer.learn_map(batch).value() << "\n";

  return 0;
}
