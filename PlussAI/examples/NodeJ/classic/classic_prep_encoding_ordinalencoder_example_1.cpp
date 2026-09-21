#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // categories arrive out of order; fitted rank follows sorted value, not arrival order.
  cls::FeatureBatch train{};
  train.x = pluss::make_series({30.0, 10.0, 40.0, 20.0}, {4}, f64).reshape({4, 1});

  cls::prep::OrdinalEncoder encoder;
  encoder.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4}, f64).reshape({4, 1});
  std::cout << "map([10, 20, 30, 40]) = " << encoder.map(probe).value() << "\n";

  return 0;
}
