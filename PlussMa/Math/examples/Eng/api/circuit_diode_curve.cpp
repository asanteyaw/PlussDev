#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  // a swept source is a batch: each lane is its own solve
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({-1.0, 0.0, 0.5, 1.0, 2.0, 5.0}).reshape({6, 1})};
  net.resistors = {{1}, {2}, vec({100.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-12});

  const auto result = ckt::op(ckt::make(net));
  out.writeln("diode_current shape", result.diode_current.sizes(), " (source value, diode)");
  const std::vector<double> volts = flat(result.voltage.select(1, 2));
  const std::vector<double> amps = flat(result.diode_current);
  out.writeln("  source   diode V   diode I (uA)");
  const double drive[] = {-1.0, 0.0, 0.5, 1.0, 2.0, 5.0};
  for (int k = 0; k < 6; ++k) out.writeln(drive[k], volts[k], amps[k] * 1e6);
}
