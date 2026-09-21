#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({0.0, 0.0, 0.0, 40.0, 0.0}, {5}, f64).reshape({5, 1});

  cls::prep::MaxAbsScaler scaler;
  scaler.learn(train);
  const pluss::Series scaled = scaler.map(train).value();

  std::cout << "map(train) = " << scaled << "\n";

  return 0;
}
