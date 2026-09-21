#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const double nan = std::numeric_limits<double>::quiet_NaN();

  // y = 2x, one missing y.
  cls::FeatureBatch train{};
  train.x = pluss::make_series(
      {1.0, 2.0, 2.0, 4.0, 3.0, 6.0, 4.0, nan}, {8}, f64).reshape({4, 2});

  cls::prep::IterativeImputerOptions options{};
  options.ridge = 1e-6;  // close to unregularized, to recover the exact relationship
  cls::prep::IterativeImputer imputer(options);
  const pluss::Series filled = imputer.learn_map(train).value();
  std::cout << "true y for x=4 is 8; filled = " << filled << "\n";

  return 0;
}
