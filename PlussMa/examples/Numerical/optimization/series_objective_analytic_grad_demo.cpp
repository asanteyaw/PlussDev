#include <pluss/pluss.h>

#include <iomanip>
#include <iostream>

using pluss::ScalarType;
using pluss::Series;
using pluss::optim::OptimaOptions;

int main() {
  const pluss::Config cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const Series target = pluss::make_series({1.0, -2.0}, cfg);
  const Series weight = pluss::make_series({1.0, 3.0}, cfg);
  const Series initial = pluss::make_series({3.5, 4.0}, cfg);

  const auto objective = [target, weight](const Series& x) -> std::pair<Series, Series> {
    const Series diff = x - target;
    const Series value = (weight * diff * diff).sum();
    const Series grad = 2.0 * weight * diff;
    return {value, grad};
  };

  OptimaOptions options = pluss::optim::presets::sequential_qp();
  options.sequential_qp.max_iterations = 80;
  options.sequential_qp.constraint_tolerance = 1e-10;
  options.sequential_qp.optimality_tolerance = 1e-10;

  const auto result = pluss::optim::optima(objective, initial, options);
  const Series x_hat = result.params.at("x").reshape({2});

  std::cout << std::fixed << std::setprecision(6);
  pluss::optim::reports::compact_summary(std::cout, result) << '\n';
  std::cout << "estimated x0=" << x_hat.select(0, 0) << '\n';
  std::cout << "estimated x1=" << x_hat.select(0, 1) << '\n';
  std::cout << "target x0=" << target.select(0, 0) << '\n';
  std::cout << "target x1=" << target.select(0, 1) << '\n';
  return 0;
}
