#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>

namespace actuarial = pluss::ma::actuarial;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const actuarial::Triangle triangle = actuarial::triangle::cumulative(
      pluss::make_series({{100.0, 150.0}, {120.0, 0.0}}));

  try {
    const actuarial::Reserving estimator =
        actuarial::operators::chain_ladder(pluss::make_series({1.50, 1.10}));
    static_cast<void>(estimator(triangle));
  } catch (const std::exception &error) {
    out.writeln("cdf rejected =", error.what());
  }
  try {
    const actuarial::Reserving estimator =
        actuarial::operators::bornhuetter_ferguson(pluss::make_series({200.0}),
                                                   pluss::make_series({0.80}));
    static_cast<void>(estimator(triangle));
  } catch (const std::exception &error) {
    out.writeln("BF input rejected =", error.what());
  }
}
