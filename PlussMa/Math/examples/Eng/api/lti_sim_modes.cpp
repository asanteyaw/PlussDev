#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const lti::StateSpace system = lti::state(
      pluss::make_series({-1.0}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}));
  const pluss::Series time = pluss::make_series({0.0, 0.5, 1.0});
  const pluss::Series ramp = pluss::make_series({0.0, 0.5, 1.0}, {3, 1});
  const pluss::Series initial = pluss::make_series({2.0});
  const lti::SimRun zero = lti::sim(
      system, ramp, time, initial, {.hold = lti::Hold::Zero});
  const lti::SimRun linear = lti::sim(
      system, ramp, time, initial, {.hold = lti::Hold::Linear});

  out.writeln("zero hold =", zero.values.flatten().items<double>());
  out.writeln("linear hold =", linear.values.flatten().items<double>());
}
