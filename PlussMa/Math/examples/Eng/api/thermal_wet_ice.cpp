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
  // Below freezing the wet bulb is an ice bulb and the energy balance changes (sublimation heat). The two branches meet at 0.
  const double p = 101325.0;
  const pluss::Series t = vec({-10.0, -2.0, 2.0, 10.0});
  const pluss::Series w = th::ratio(t, vec({0.5}), vec({p}));
  out.writeln("dry bulb", flat(t));
  out.writeln("wet bulb", flat(th::wet(t, w, vec({p}))));
}
