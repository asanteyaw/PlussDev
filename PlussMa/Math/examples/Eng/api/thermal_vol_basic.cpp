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
  // Specific volume (m^3 per kg of dry air) and the density of the moist air it implies.
  const double p = 101325.0;
  const pluss::Series v = th::vol(vec({25.0}), vec({0.010}), vec({p}));
  out.writeln("specific volume", flat(v), " closed form", 287.042 * 298.15 * (1.0 + 1.607858 * 0.010) / p);
  out.writeln("density of the mixture", (1.0 + 0.010) / one(v), "kg/m^3");
  out.writeln("dry air at 25 degrees:", flat(th::vol(vec({25.0}), vec({0.0}), vec({p}))));
}
