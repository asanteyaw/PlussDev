#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace lti = pluss::ma::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const lti::StateSpace system = lti::state(
      pluss::make_series({-1.0}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}));
  const pluss::Series time = pluss::make_series({0.0, 0.2, 0.7, 1.5});
  const pluss::Series input = pluss::make_series({1.0, 1.0, 1.0, 1.0}, {4, 1});
  const lti::SimRun run = lti::sim(
      system, input, time, {.hold = lti::Hold::Zero});

  out.writeln("values =", run.values.flatten().items<double>());
  out.writeln("states =", run.states.flatten().items<double>());
}
