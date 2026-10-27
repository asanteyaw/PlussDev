// The objective and constraints can be bundled into one ConstrainedProblem value, either by
// filling the struct directly or with the problem() builder. Both routes describe the same
// problem; the builder is shorter and picks the right objective slot for the callable you give it.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto target = vec({1.0, 2.0});
  auto objective = [target](const Pack& p) { return (p.at("x") - target).square().sum(); };
  auto budget = [](const Pack& p) { return p.at("x").sum() - 1.5; };  // x0 + x1 <= 1.5

  // Route 1: fill the struct.
  optim::ConstrainedProblem by_hand;
  by_hand.objective.autodiff = objective;
  by_hand.nonlinear.inequality = budget;

  // Route 2: the builder.
  const optim::ConstrainedProblem built = optim::problem().objective(objective).nl_ineq(budget).build();

  const auto start = vec({0.0, 0.0});
  const auto a = optim::optima(by_hand, start);                           // start as a plain vector
  const auto b = optim::optima(built, optim::pack_series(start));        // start as a named-block pack

  const auto xa = a.params.at("x").host<double>();
  const auto xb = b.params.at("x").host<double>();
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "struct:  x = [" << xa[0] << ", " << xa[1] << "]\n";
  std::cout << "builder: x = [" << xb[0] << ", " << xb[1] << "]\n";
  std::cout << "same answer: " << std::boolalpha << (xa == xb) << "\n";
  std::cout << "pack start returns block names: " << b.params.contains("x") << "\n";
  return 0;
}
