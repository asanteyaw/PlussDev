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
  // The dew point of air at 25 degrees and 50 % relative humidity, and the check that the air is saturated there.
  const double p = 101325.0;
  const pluss::Series w = th::ratio(vec({25.0}), vec({0.5}), vec({p}));
  const pluss::Series td = th::dew(w, vec({p}));
  out.writeln("dew point", flat(td), "degrees");
  out.writeln("relative humidity at the dew point", flat(th::rh(td, w, vec({p}))));
}
