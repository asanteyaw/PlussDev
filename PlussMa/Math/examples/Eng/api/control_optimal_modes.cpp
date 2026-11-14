#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const pluss::Series a = pluss::make_series({0.5}, {1, 1});
  const pluss::Series b = pluss::make_series({1.0}, {1, 1});
  const pluss::Series q = pluss::make_series({1.0}, {1, 1});
  const pluss::Series r = pluss::make_series({1.0}, {1, 1});
  const control::RiccatiResult direct = control::dare(a, b, q, r);
  const lti::StateSpace plant = lti::state(
      a, b, pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({0.0}, {1, 1}), 1.0);
  const control::RiccatiResult routed = control::lqr(plant, q, r);

  out.writeln("direct value =", direct.value.flatten().items<double>());
  out.writeln("routed value =", routed.value.flatten().items<double>());
  out.writeln("discrete gain =", routed.gain.flatten().items<double>());
}
