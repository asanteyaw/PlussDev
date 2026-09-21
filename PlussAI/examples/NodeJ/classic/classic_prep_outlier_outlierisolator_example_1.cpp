#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series(
      {0.0, 0.1, -0.1, 0.05, -0.05, 0.2, -0.2, 0.0, 0.1, 100.0},
      {10}, f64).reshape({10, 1});

  cls::prep::OutlierIsolatorOptions options{};
  options.random = cls::RandomState{};  // seed = 0, deterministic -- the default
  cls::prep::OutlierIsolator isolator(options);
  isolator.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({0.0, 100.0}, {2}, f64).reshape({2, 1});
  std::cout << "scores (normal point, then the outlier) = " << isolator.score_samples(probe).value() << "\n";

  return 0;
}
