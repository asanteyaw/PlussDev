#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const lti::StateSpace model = lti::state(
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}),
      1.0);
  const control::SmoothResult result = control::smooth(
      model, pluss::make_series({0.2, 0.1, -0.3, 0.4}, {4, 1}),
      pluss::make_series({0.04}, {1, 1}),
      pluss::make_series({0.25}, {1, 1}), pluss::make_series({0.0}),
      pluss::make_series({1.0}, {1, 1}), {.jitter = 0.0});

  out.writeln("filtered mean =",
              result.filter.filtered_mean.flatten().items<double>());
  out.writeln("smoothed mean =", result.smoothed_mean.flatten().items<double>());
  out.writeln("final covariance =",
              result.smoothed_cov.select(-3, 3).flatten().items<double>());
}
