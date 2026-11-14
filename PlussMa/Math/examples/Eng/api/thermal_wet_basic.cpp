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
  // The wet-bulb temperature of air at 25 degrees and 50 %: what a wetted thermometer reads in a fast stream.
  const double p = 101325.0;
  const pluss::Series w = th::ratio(vec({25.0}), vec({0.5}), vec({p}));
  const pluss::Series wb = th::wet(vec({25.0}), w, vec({p}));
  out.writeln("dry bulb 25  wet bulb", flat(wb), "  dew point", flat(th::dew(w, vec({p}))));
  out.writeln("the ordering dew point < wet bulb < dry bulb holds for unsaturated air");
}
