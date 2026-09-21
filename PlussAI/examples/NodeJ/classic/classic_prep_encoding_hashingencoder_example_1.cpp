#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::HashingEncoderOptions options{};
  options.n_components = 4;

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}, {5}, f64).reshape({5, 1});

  cls::prep::HashingEncoder first(options);
  first.learn(probe);
  cls::prep::HashingEncoder second(options);
  second.learn(probe);

  std::cout << "first  = " << first.map(probe).value() << "\n";
  std::cout << "second = " << second.map(probe).value() << "\n";

  return 0;
}
