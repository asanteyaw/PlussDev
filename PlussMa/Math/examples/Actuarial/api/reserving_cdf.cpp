#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));

  const actuarial::Triangle triangle =
      actuarial::triangle::cumulative(pluss::make_series(
          {{100.0, 150.0, 180.0}, {120.0, 180.0, 0.0}, {130.0, 0.0, 0.0}}));
  const actuarial::Reserving estimator = actuarial::operators::chain_ladder(
      pluss::make_series({1.80, 1.20, 1.00}));
  const actuarial::ReserveResult result = estimator(triangle);

  out.writeln("supplied cdf =", result.cdf.items<double>());
  out.writeln("ultimate =", result.ultimate.items<double>());
  out.writeln("reserve =", result.reserve.items<double>());
}
