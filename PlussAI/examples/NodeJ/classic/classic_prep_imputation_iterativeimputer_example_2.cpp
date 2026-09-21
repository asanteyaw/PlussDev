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
  train.x = pluss::make_series({10.0, 20.0, 30.0, nan}, {4}, f64).reshape({4, 1});  // mean of 10,20,30 = 20

  cls::prep::IterativeImputer imputer;
  const pluss::Series filled = imputer.learn_map(train).value();
  std::cout << "single feature -> plain mean fill: " << filled << "\n";

  return 0;
}
