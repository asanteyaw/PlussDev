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
  train.x = pluss::make_series(
      {0.0, 0.0, 1.0, 100.0, 10.0, 200.0}, {6}, f64).reshape({3, 2});

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({0.5, nan}, {2}, f64).reshape({1, 2});

  cls::prep::NeighborImputerOptions uniform_opts{};
  uniform_opts.neighbors = 3;
  uniform_opts.weighting = cls::prep::NeighborImputerWeighting::Uniform;
  cls::prep::NeighborImputer uniform(uniform_opts);
  uniform.learn(train);
  std::cout << "Uniform:  " << uniform.map(probe).value() << "\n";

  cls::prep::NeighborImputerOptions distance_opts{};
  distance_opts.neighbors = 3;
  distance_opts.weighting = cls::prep::NeighborImputerWeighting::Distance;
  cls::prep::NeighborImputer distance(distance_opts);
  distance.learn(train);
  std::cout << "Distance: " << distance.map(probe).value() << "\n";

  return 0;
}
