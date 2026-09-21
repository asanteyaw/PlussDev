#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({2.0, -8.0, 4.0}, {3}, f64).reshape({3, 1});  // max |value| = 8

  cls::prep::MaxAbsScaler scaler;
  scaler.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({8.0, -8.0, 4.0, 0.0}, {4}, f64).reshape({4, 1});
  std::cout << "max|value| = 8, map([8, -8, 4, 0]) = " << scaler.map(probe).value() << "\n";

  return 0;
}
