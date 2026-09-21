#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // one feature column: categories encoded as small integers 1, 2, 3.
  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 3.0, 1.0, 2.0}, {5}, f64).reshape({5, 1});

  cls::prep::OneHotEncoder encoder;
  encoder.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  std::cout << "map([1, 2, 3]) = " << encoder.map(probe).value() << "\n";

  return 0;
}
