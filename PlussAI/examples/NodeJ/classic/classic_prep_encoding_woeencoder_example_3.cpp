#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // category 1 is purely positive; category 2 provides the only negative rows.
  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 1.0, 2.0, 2.0}, {4}, f64).reshape({4, 1});
  const pluss::Series target = pluss::make_series({1.0, 1.0, 0.0, 0.0}, {4}, f64);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0}, {1}, f64).reshape({1, 1});

  cls::prep::WOEEncoderOptions zero_reg{};
  zero_reg.regularization = 0.0;
  cls::prep::WOEEncoder degenerate(zero_reg);
  degenerate.learn_supervised(train, target);
  std::cout << "regularization=0:   " << degenerate.map(probe).value() << "\n";

  cls::prep::WOEEncoderOptions with_reg{};
  with_reg.regularization = 0.5;
  cls::prep::WOEEncoder regularized(with_reg);
  regularized.learn_supervised(train, target);
  std::cout << "regularization=0.5: " << regularized.map(probe).value() << "\n";

  return 0;
}
