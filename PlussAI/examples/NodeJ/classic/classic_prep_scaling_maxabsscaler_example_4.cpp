#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({0.0, 0.0, 0.0}, {3}, f64).reshape({3, 1});  // all-zero

  cls::prep::MaxAbsScaler scaler;
  scaler.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({0.0}, {1}, f64).reshape({1, 1});
  std::cout << "all-zero feature, map([0]) = " << scaler.map(probe).value() << "\n";

  return 0;
}
