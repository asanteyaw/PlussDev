#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>
#include <vector>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const double nan = std::numeric_limits<double>::quiet_NaN();

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({nan, 3.0}, {2}, f64).reshape({1, 2});

  cls::prep::PolynomialLiftOptions options{};
  options.degree = 2;
  options.include_bias = true;
  cls::prep::PolynomialLift lift(options);
  const pluss::Series mapped = lift.learn_map(batch).value();

  // Series' own printer truncates a wide row; unpack it to see every term.
  const std::vector<double> terms = mapped.to(pluss::pCPU, pluss::ScalarType::Float64).reshape({mapped.numel()}).to_vec<double>();
  const char* names[] = {"bias", "x0", "x1", "x0^2", "x0*x1", "x1^2"};
  for (std::size_t i = 0; i < terms.size(); ++i) {
    std::cout << names[i] << " = " << terms[i] << "\n";
  }

  return 0;
}
