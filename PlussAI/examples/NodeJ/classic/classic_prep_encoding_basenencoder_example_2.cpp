#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const double nan = std::numeric_limits<double>::quiet_NaN();

  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 20.0}, {2}, f64).reshape({2, 1});
  cls::prep::BaseNEncoder encoder;
  encoder.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({99.0, nan}, {2}, f64).reshape({2, 1});
  std::cout << "unseen (99) and missing (NaN) both -> " << encoder.map(probe).value() << "\n";

  return 0;
}
