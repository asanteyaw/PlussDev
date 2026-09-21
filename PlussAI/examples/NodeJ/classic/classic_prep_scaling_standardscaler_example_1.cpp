#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // One feature column: values 2, 4, 4, 4, 5, 5, 7, 9 -- mean 5, population stddev 2.
  cls::FeatureBatch train{};
  train.x = pluss::make_series({2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0}, {8}, f64).reshape({8, 1});

  cls::prep::StandardScaler scaler;
  scaler.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({5.0, 9.0, 1.0}, {3}, f64).reshape({3, 1});
  const pluss::Series scaled = scaler.map(probe).value();

  std::cout << "mean = 5, stddev = 2\n";
  std::cout << "map([5, 9, 1]) = " << scaled << "\n";

  return 0;
}
