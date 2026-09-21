#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const double nan = std::numeric_limits<double>::quiet_NaN();

  // feature 0 is the "similarity" axis; feature 1 is what we fill.
  cls::FeatureBatch train{};
  train.x = pluss::make_series(
      {0.0, 10.0, 1.0, 11.0, 5.0, 50.0, 6.0, 51.0}, {8}, f64).reshape({4, 2});

  cls::prep::NeighborImputerOptions options{};
  options.neighbors = 2;
  cls::prep::NeighborImputer imputer(options);
  imputer.learn(train);

  // query is closest (on feature 0) to the first two training rows.
  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({0.5, nan}, {2}, f64).reshape({1, 2});
  std::cout << "map([0.5, NaN]) = " << imputer.map(probe).value() << "\n";

  return 0;
}
