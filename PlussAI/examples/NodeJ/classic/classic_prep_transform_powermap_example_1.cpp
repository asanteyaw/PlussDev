#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0}, {7}, f64).reshape({7, 1});

  cls::prep::PowerMap power_map;  // YeoJohnson, standardize = true by default
  const pluss::Series reshaped = power_map.learn_map(train).value();
  std::cout << "reshaped = " << reshaped << "\n";

  return 0;
}
