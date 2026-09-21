#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({3.0, 3.0, 3.0, 3.0}, {4}, f64).reshape({4, 1});  // constant

  cls::prep::RobustScaler scaler;
  scaler.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({3.0, 50.0}, {2}, f64).reshape({2, 1});
  std::cout << "constant feature, map([3, 50]) = " << scaler.map(probe).value() << "\n";

  return 0;
}
