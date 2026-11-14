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
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-14});
  const auto circuit = ckt::make(net);
  // thermal voltage k T / q: 25.85 mV at 300 K, about 34.5 mV at 400 K, 21.5 mV at 250 K
  for (double vt : {0.02154, 0.02585, 0.03447}) {
    ckt::SolveOptions options;
    options.thermal_voltage = vt;
    out.writeln("thermal voltage", vt, " diode voltage", flat(ckt::op(circuit, options).voltage)[2]);
  }
}
