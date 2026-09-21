#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const double nan = std::numeric_limits<double>::quiet_NaN();

  // feature 0 is constant (every value 7); feature 1 has real variance and one missing entry.
  cls::FeatureBatch train{};
  train.x = pluss::make_series({7.0, 1.0, 7.0, nan, 7.0, 7.0}, {6}, f64).reshape({3, 2});

  cls::prep::StandardScaler scaler;
  scaler.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({7.0, nan, 10.0, 3.0}, {4}, f64).reshape({2, 2});
  const pluss::Series scaled = scaler.map(probe).value();

  std::cout << "constant feature 0, missing probe entry: map = " << scaled << "\n";

  return 0;
}
