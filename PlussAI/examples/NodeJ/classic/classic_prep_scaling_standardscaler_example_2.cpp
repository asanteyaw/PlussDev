#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0}, {8}, f64).reshape({8, 1});

  cls::prep::StandardScalerOptions options{};
  options.with_centering = false;
  cls::prep::StandardScaler scaler(options);
  scaler.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({10.0}, {1}, f64).reshape({1, 1});
  std::cout << "with_centering=false, map([10]) = " << scaler.map(probe).value() << "\n";

  return 0;
}
