#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}, {5}, f64).reshape({5, 1});

  cls::prep::QuantileMapOptions options{};
  options.output = cls::prep::QuantileMapOutput::Gaussian;
  cls::prep::QuantileMap map(options);
  std::cout << "Gaussian output = " << map.learn_map(train).value() << "\n";

  return 0;
}
