#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <iostream>

namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const lti::StateSpace system = lti::state(
      pluss::make_series({{0.0, 1.0}, {-2.0, -3.0}}),
      pluss::make_series({0.0, 1.0}, {2, 1}),
      pluss::make_series({{1.0, 0.0}}),
      pluss::make_series({0.0}, {1, 1}));

  std::cout << "states = " << system.states()
            << ", inputs = " << system.inputs()
            << ", outputs = " << system.outputs() << "\n";
  out.writeln("continuous =", system.is_cont());
  out.writeln("controllability =", lti::ctrb(system).flatten().items<double>());
  out.writeln("observability =", lti::obsv(system).flatten().items<double>());
}
