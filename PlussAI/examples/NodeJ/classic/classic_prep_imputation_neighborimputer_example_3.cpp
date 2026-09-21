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
  train.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});  // mean = 20

  cls::prep::NeighborImputer imputer;
  imputer.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({nan}, {1}, f64).reshape({1, 1});  // no other feature to compare on
  std::cout << "no overlapping feature -> " << imputer.map(probe).value() << "\n";

  return 0;
}
