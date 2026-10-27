// A hook can be set once for any algorithm on OptimaOptions::progress_hook, or on one algorithm's own
// option block. The algorithm-specific hook wins when both are set; the general one fills in when the
// algorithm's own is empty. Counting calls shows which hook ran.
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const pluss::Series& x) { return (x - 3.0).square().sum(); };

  int general = 0, specific = 0;
  optim::OptimaOptions options;
  options.progress_hook = [&](const optim::OptimaProgressSnapshot&) { ++general; return false; };
  (void)optim::optima(objective, vec({0.0, 0.0}), options);
  std::cout << "general hook only:      general=" << general << " specific=" << specific << "\n";

  general = 0;
  options.sequential_qp.progress_hook = [&](const optim::OptimaProgressSnapshot&) { ++specific; return false; };
  (void)optim::optima(objective, vec({0.0, 0.0}), options);
  std::cout << "both hooks set:         general=" << general << " specific=" << specific << "\n";

  general = specific = 0;
  options.algorithm = optim::OptimaAlgorithm::WorkingSet;
  (void)optim::optima(objective, vec({0.0, 0.0}), options);
  std::cout << "other algorithm chosen: general=" << general << " specific=" << specific << "\n";
  return 0;
}
