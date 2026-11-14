#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // A sweep of humidity ratios, the dew point of each, and the saturation pressure there equals the vapour pressure.
  const double p = 101325.0;
  const pluss::Series w = vec({0.002, 0.005, 0.010, 0.020});
  const pluss::Series td = th::dew(w, vec({p}));
  out.writeln("dew points", flat(td));
  out.writeln("psat at the dew point", flat(th::psat(td)));
  out.writeln("vapour pressure p w / (0.621945 + w)", flat(w.mul(p).div(w.add(0.621945))));
}
