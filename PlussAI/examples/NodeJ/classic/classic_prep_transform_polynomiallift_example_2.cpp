#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({2.0, 3.0}, {2}, f64).reshape({1, 2});

  cls::prep::PolynomialLiftOptions options{};
  options.degree = 2;
  options.interactions_only = true;
  cls::prep::PolynomialLift lift(options);
  std::cout << "interactions_only, x0=2, x1=3 -> " << lift.learn_map(batch).value() << "\n";

  return 0;
}
