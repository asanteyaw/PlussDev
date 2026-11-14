#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static double one(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>()[0]; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // Humidity ratio of air at 25 degrees and 50 % relative humidity at sea-level pressure.
  const double p = 101325.0;
  const pluss::Series w = th::ratio(vec({25.0}), vec({0.5}), vec({p}));
  const double pw = 0.5 * one(th::psat(vec({25.0})));
  out.writeln("vapour pressure", pw, "Pa  w =", flat(w), "kg/kg  closed form", 0.621945 * pw / (p - pw));
  out.writeln("saturated at 25 degrees:", flat(th::ratio(vec({25.0}), vec({1.0}), vec({p}))));
  out.writeln("dry air:", flat(th::ratio(vec({25.0}), vec({0.0}), vec({p}))));
}
