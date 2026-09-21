#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // five ordinary points clustered around 10, then one wild outlier.
  cls::FeatureBatch train{};
  train.x = pluss::make_series({9.0, 10.0, 10.0, 11.0, 10.0, 1000.0}, {6}, f64).reshape({6, 1});

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({10.0}, {1}, f64).reshape({1, 1});  // a typical, non-outlier value

  cls::prep::RobustScaler robust;
  robust.learn(train);
  cls::prep::StandardScaler standard;
  standard.learn(train);

  std::cout << "RobustScaler::map(10)   = " << robust.map(probe).value() << "\n";
  std::cout << "StandardScaler::map(10) = " << standard.map(probe).value() << "\n";

  return 0;
}
