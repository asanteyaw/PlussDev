#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const lti::StateSpace plant = lti::state(
      pluss::make_series({-1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({0.0}, {1, 1}));
  const control::LqeResult result = control::lqe(
      plant, pluss::make_series({0.25}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}));

  out.writeln("error covariance =", result.covariance.flatten().items<double>());
  out.writeln("estimator gain =", result.gain.flatten().items<double>());
  out.writeln("error pole =", result.closed_loop.flatten().items<double>());
}
