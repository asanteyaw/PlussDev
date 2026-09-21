#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series(
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0}, {10}, f64).reshape({10, 1});

  cls::prep::RobustScalerOptions options{};
  options.lower_quantile = 0.1;
  options.upper_quantile = 0.9;
  cls::prep::RobustScaler scaler(options);
  scaler.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({10.0}, {1}, f64).reshape({1, 1});
  std::cout << "quantiles=[0.1, 0.9], map([10]) = " << scaler.map(probe).value() << "\n";

  return 0;
}
