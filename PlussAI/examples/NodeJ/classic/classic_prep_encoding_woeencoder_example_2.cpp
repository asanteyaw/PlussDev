#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  const pluss::Series target = pluss::make_series({0.0, 1.0, 2.0}, {3}, f64);  // 2.0 is not binary

  cls::prep::WOEEncoder encoder;
  const cls::Status fit = encoder.learn_supervised(train, target);
  std::cout << "ok = " << std::boolalpha << fit.ok() << "\n";
  std::cout << "message = \"" << fit.message << "\"\n";

  return 0;
}
