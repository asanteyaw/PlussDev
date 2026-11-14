#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const actuarial::Mortality mortality = actuarial::mortality::from_qx(
      pluss::make_series({0.05, 0.10, 0.20, 0.40}));

  out.writeln("force =", mortality.force().items<double>());
  out.writeln("survival =", mortality.survival().items<double>());
  out.writeln("life table =", mortality.life_table(1000.0).items<double>());
  out.writeln("lifetime pmf =", mortality.lifetime_pmf().items<double>());
}
