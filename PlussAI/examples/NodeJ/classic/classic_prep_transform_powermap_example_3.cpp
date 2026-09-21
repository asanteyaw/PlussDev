#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});

  cls::prep::PowerMapOptions options{};
  options.mode = cls::prep::PowerMapMode::BoxCox;
  options.standardize = false;  // isolate the power transform itself
  options.lambda = 1.0;
  cls::prep::PowerMap power_map(options);
  std::cout << "lambda=1 (BoxCox, no standardize) = " << power_map.learn_map(train).value() << "\n";

  return 0;
}
