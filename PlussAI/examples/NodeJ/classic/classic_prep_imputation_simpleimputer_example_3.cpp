#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const double nan = std::numeric_limits<double>::quiet_NaN();

  cls::FeatureBatch train{};
  train.x = pluss::make_series({nan, nan, nan}, {3}, f64).reshape({3, 1});

  cls::prep::SimpleImputerOptions options{};
  options.strategy = cls::prep::SimpleImputerStrategy::Mean;
  options.empty_feature_fallback = 42.0;
  cls::prep::SimpleImputer imputer(options);
  imputer.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({nan}, {1}, f64).reshape({1, 1});
  std::cout << "entirely-missing feature -> " << imputer.map(probe).value() << "\n";

  return 0;
}
