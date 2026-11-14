#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static double one(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>()[0]; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // Sensible and latent parts of a cooling load: cooling 30 degrees, w = 0.014 to 15 degrees, w = 0.0106 for 1 kg/s of dry air.
  const double h1 = one(th::enth(vec({30.0}), vec({0.014}))), h2 = one(th::enth(vec({15.0}), vec({0.0106})));
  const double sensible = 1006.0 * 15.0 + 0.0106 * 1860.0 * 15.0;
  out.writeln("total load", h1 - h2, "W   sensible", sensible, "W   latent", (0.014 - 0.0106) * (2501000.0 + 1860.0 * 30.0), "W");
}
