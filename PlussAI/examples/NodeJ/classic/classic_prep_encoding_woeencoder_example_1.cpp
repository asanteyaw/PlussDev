#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // category 1: targets 1, 1 (both positive). category 2: targets 0, 0 (both negative).
  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 1.0, 2.0, 2.0}, {4}, f64).reshape({4, 1});
  const pluss::Series target = pluss::make_series({1.0, 1.0, 0.0, 0.0}, {4}, f64);

  cls::prep::WOEEncoder encoder;
  encoder.learn_supervised(train, target);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  std::cout << "map([category 1, category 2]) = " << encoder.map(probe).value() << "\n";

  return 0;
}
