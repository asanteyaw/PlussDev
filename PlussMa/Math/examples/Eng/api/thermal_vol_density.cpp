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
  // Humid air is lighter than dry air at the same temperature and pressure.
  const double p = 101325.0;
  const pluss::Series dry = th::vol(vec({25.0}), vec({0.0}), vec({p})), humid = th::vol(vec({25.0}), vec({0.020}), vec({p}));
  out.writeln("dry density ", 1.0 / one(dry), "kg/m^3");
  out.writeln("humid density", 1.020 / one(humid), "kg/m^3 (w = 0.020), lighter by", 1.0 - (1.020 / one(humid)) * one(dry), "of the dry value");
}
