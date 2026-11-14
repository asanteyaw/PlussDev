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
  // Saturated air: all three temperatures coincide. Drier air widens the gap.
  const double p = 101325.0;
  for (const double phi : {1.0, 0.8, 0.5, 0.2}) {
    const pluss::Series w = th::ratio(vec({30.0}), vec({phi}), vec({p}));
    out.writeln("relative humidity", phi, " wet bulb", flat(th::wet(vec({30.0}), w, vec({p}))), " dew point", flat(th::dew(w, vec({p}))));
  }
}
