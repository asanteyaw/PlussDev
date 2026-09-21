#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({0.0, 40.0}, {2}, f64).reshape({2, 1});  // outside [10, 30]

  cls::prep::MinMaxScaler unclipped;
  unclipped.learn(train);
  std::cout << "clip=false, map([0, 40]) = " << unclipped.map(probe).value() << "\n";

  cls::prep::MinMaxScalerOptions options{};
  options.clip = true;
  cls::prep::MinMaxScaler clipped(options);
  clipped.learn(train);
  std::cout << "clip=true,  map([0, 40]) = " << clipped.map(probe).value() << "\n";

  return 0;
}
