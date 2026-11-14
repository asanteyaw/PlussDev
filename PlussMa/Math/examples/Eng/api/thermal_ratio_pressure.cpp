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
  // The same temperature and relative humidity at altitude hold more water per kilogram of dry air: the total pressure is lower.
  const pluss::Series p = vec({101325.0, 89875.0, 79495.0, 54020.0});
  out.writeln("pressure (sea level, 1000 m, 2000 m, 5000 m)", flat(p));
  out.writeln("w at 20 degrees, 50 %", flat(th::ratio(vec({20.0}), vec({0.5}), p)));
}
