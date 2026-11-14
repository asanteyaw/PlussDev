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
  // Air density follows the pressure; the transport properties do not. Water ignores the pressure argument.
  const pluss::Series t = vec({300.0});
  const auto atm = th::props(th::Fluid::Air, t);
  const auto high = th::props(th::Fluid::Air, t, vec({5.0 * 101325.0}));
  out.writeln("air density  1 atm", flat(atm.density), "  5 atm", flat(high.density));
  out.writeln("air viscosity (uPa s) 1 atm", flat(atm.viscosity.mul(1e6)), "  5 atm", flat(high.viscosity.mul(1e6)));
  out.writeln("water density", flat(th::props(th::Fluid::Water, t).density), "  with a pressure", flat(th::props(th::Fluid::Water, t, vec({5.0e6})).density));
}
