#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // the emission coefficient widens the knee: a larger n needs a larger voltage for the same current
  for (double n : {1.0, 1.5, 2.0}) {
    ckt::Net net;
    net.nodes = 3;
    net.voltage_sources = {{1}, {0}, vec({5.0})};
    net.resistors = {{1}, {2}, vec({1000.0})};
    net.diodes.positive = {2};
    net.diodes.negative = {0};
    net.diodes.saturation = vec({1e-14});
    net.diodes.ideality = vec({n});
    const auto result = ckt::op(ckt::make(net));
    out.writeln("ideality", n, " diode voltage", flat(result.voltage)[2], " current", flat(result.diode_current)[0]);
  }
}
