#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});

  cls::prep::HashingEncoderOptions seed0{};
  seed0.n_components = 4;
  cls::prep::HashingEncoder encoder0(seed0);
  encoder0.learn(probe);

  cls::prep::HashingEncoderOptions seed7 = seed0;
  seed7.seed = 7;
  cls::prep::HashingEncoder encoder7(seed7);
  encoder7.learn(probe);

  std::cout << "seed=0 -> " << encoder0.map(probe).value() << "\n";
  std::cout << "seed=7 -> " << encoder7.map(probe).value() << "\n";

  return 0;
}
