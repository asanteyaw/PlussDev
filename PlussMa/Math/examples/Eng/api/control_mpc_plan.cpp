#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <iostream>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const lti::StateSpace plant = lti::state(
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}),
      1.0);
  const control::mpc::Weights weights{
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.1}, {1, 1})};
  const control::mpc::Bounds bounds{
      pluss::make_series({-0.5}), pluss::make_series({0.5})};
  const control::mpc::Plan plan =
      control::mpc::build(plant, weights, bounds, {.horizon = 3});
  const control::mpc::Result first = control::mpc::solve(
      plan, pluss::make_series({0.0}), pluss::make_series({1.0}));
  const control::mpc::Result warm = control::mpc::solve(
      plan, pluss::make_series({0.1}), pluss::make_series({1.0}),
      first.optimization.warm());

  std::cout << "horizon = " << plan.horizon() << ", states = " << plan.states()
            << ", inputs = " << plan.inputs() << "\n";
  out.writeln("cold iterations =", first.optimization.iterations);
  out.writeln("warm iterations =", warm.optimization.iterations);
  out.writeln("warm controls =", warm.controls.flatten().items<double>());
}
