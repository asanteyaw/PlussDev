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
  // Enthalpy of air at 25 degrees with w = 0.010, per kg of dry air; zero is dry air at 0 degrees and liquid water at 0 degrees.
  const pluss::Series h = th::enth(vec({25.0}), vec({0.010}));
  out.writeln("enthalpy", flat(h), "J/kg  closed form", 1006.0 * 25.0 + 0.010 * (2501000.0 + 1860.0 * 25.0));
  out.writeln("dry air at 0 degrees:", flat(th::enth(vec({0.0}), vec({0.0}))), " saturated vapour only at 0 degrees, w = 0.01:", flat(th::enth(vec({0.0}), vec({0.010}))));
}
