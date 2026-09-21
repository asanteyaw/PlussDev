#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}, {5}, f64).reshape({5, 1});

  cls::prep::QuantileMap map;  // output = Uniform by default
  const pluss::Series percentiles = map.learn_map(train).value();
  std::cout << "percentiles = " << percentiles << "\n";

  return 0;
}
