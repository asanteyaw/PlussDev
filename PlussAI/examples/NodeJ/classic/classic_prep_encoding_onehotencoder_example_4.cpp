#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // feature 0: 5 distinct categories (over the cap of 2); feature 1: 2 categories.
  cls::FeatureBatch train{};
  train.x = pluss::make_series(
      {1.0, 10.0, 2.0, 20.0, 3.0, 10.0, 4.0, 20.0, 5.0, 10.0}, {10}, f64).reshape({5, 2});

  cls::prep::OneHotEncoderOptions options{};
  options.max_categories = 2;
  options.overflow = cls::prep::CategoryOverflowPolicy::SkipFeature;
  cls::prep::OneHotEncoder encoder(options);
  encoder.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0, 10.0}, {2}, f64).reshape({1, 2});
  std::cout << "map([1, 10]) = " << encoder.map(probe).value() << "\n";

  return 0;
}
