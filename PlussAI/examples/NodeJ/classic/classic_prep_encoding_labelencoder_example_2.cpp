#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});

  cls::prep::LabelEncoderOptions options{};
  options.one_based = true;
  cls::prep::LabelEncoder encoder(options);
  encoder.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});
  std::cout << "one_based=true, map([10, 20, 30]) = " << encoder.map(probe).value() << "\n";

  return 0;
}
