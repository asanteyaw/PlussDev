// Watching the iterations. `reports::iter` turns any output stream into a progress hook that prints one line per
// iteration: the iteration number, the evaluations so far, the objective, the constraint violation, the stationarity
// measure, the algorithm's own parameter, the step and the stall category. It is the same problem as the previous program (the point of the disc x^2 + y^2 <= 1 closest to (1, 2)), so the lines show the solver
// walking from the origin to the circle.
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };

  const auto target = vec({1.0, 2.0});
  auto objective = [&](const pluss::Series& x) { return (x - target).square().sum(); };
  const auto disc = optim::constraints().nl_ineq(
      [](const optim::ParameterPack<pluss::Series>& p) { return p.at("x").square().sum() - 1.0; });

  optim::OptimaOptions options;
  options.progress_hook = optim::reports::iter(std::cout, 4);  // four decimals per number
  const auto result = optim::optima(objective, vec({0.0, 0.0}), options, disc);

  std::cout << "converged: " << std::boolalpha << result.converged << " after " << result.iteration << " iterations\n";
  return 0;
}
