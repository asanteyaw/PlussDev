#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 20.0}, {2}, f64).reshape({2, 1});  // 2 categories -> codes 1, 2

  cls::prep::BaseNEncoderOptions options{};
  options.base = 3;
  cls::prep::BaseNEncoder encoder(options);
  encoder.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({10.0, 20.0}, {2}, f64).reshape({2, 1});
  std::cout << "base=3, codes 1 and 2 -> " << encoder.map(probe).value() << "\n";

  return 0;
}
