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
  train.x = pluss::make_series({1.0, 1.0, 2.0, nan}, {4}, f64).reshape({4, 1});

  cls::prep::SimpleImputerOptions freq_opts{};
  freq_opts.strategy = cls::prep::SimpleImputerStrategy::MostFrequent;
  cls::prep::SimpleImputer freq_imputer(freq_opts);
  freq_imputer.learn(train);
  std::cout << "MostFrequent: " << freq_imputer.map(train).value() << "\n";

  cls::prep::SimpleImputerOptions const_opts{};
  const_opts.strategy = cls::prep::SimpleImputerStrategy::Constant;
  const_opts.constant_value = -1.0;
  cls::prep::SimpleImputer const_imputer(const_opts);
  const_imputer.learn(train);
  std::cout << "Constant(-1): " << const_imputer.map(train).value() << "\n";

  return 0;
}
