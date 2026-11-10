#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace lti = pluss::ma::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const lti::StateSpace continuous = lti::state(
      pluss::make_series({-1.0}, {1, 1}), pluss::make_series({2.0}, {1, 1}),
      pluss::make_series({3.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}));
  const lti::StateSpace discrete = lti::state(
      pluss::make_series({0.5}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}), 0.25);

  out.writeln("continuous reach gram =",
              lti::gram(continuous, lti::Gram::Reach).flatten().items<double>());
  out.writeln("continuous observe gram =",
              lti::gram(continuous, lti::Gram::Observe).flatten().items<double>());
  out.writeln("discrete reach gram =",
              lti::gram(discrete, lti::Gram::Reach).flatten().items<double>());
}
