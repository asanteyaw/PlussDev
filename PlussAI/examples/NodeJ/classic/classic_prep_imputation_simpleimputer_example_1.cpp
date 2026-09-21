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
  train.x = pluss::make_series({1.0, 2.0, 3.0, 100.0, nan}, {5}, f64).reshape({5, 1});

  cls::prep::SimpleImputer mean_imputer;  // strategy = Mean by default
  mean_imputer.learn(train);
  std::cout << "Mean:   " << mean_imputer.map(train).value() << "\n";

  cls::prep::SimpleImputerOptions median_opts{};
  median_opts.strategy = cls::prep::SimpleImputerStrategy::Median;
  cls::prep::SimpleImputer median_imputer(median_opts);
  median_imputer.learn(train);
  std::cout << "Median: " << median_imputer.map(train).value() << "\n";

  return 0;
}
