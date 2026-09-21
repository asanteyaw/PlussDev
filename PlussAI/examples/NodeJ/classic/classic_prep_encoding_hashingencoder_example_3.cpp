#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const double nan = std::numeric_limits<double>::quiet_NaN();

  cls::prep::HashingEncoderOptions options{};
  options.n_components = 4;
  cls::prep::HashingEncoder encoder(options);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0, nan}, {2}, f64).reshape({2, 1});
  encoder.learn(probe);
  std::cout << "map([1, NaN]) = " << encoder.map(probe).value() << "\n";

  return 0;
}
