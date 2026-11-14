#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  ckt::Net amplifier;
  amplifier.nodes = 3;
  amplifier.voltage_sources = {{1}, {0}, vec({2.0})};
  amplifier.resistors = {{2}, {0}, vec({1000.0})};
  amplifier.vcvs = {{2}, {0}, {1}, {0}, vec({5.0})};
  const auto voltage_gain = ckt::op(ckt::make(amplifier));
  out.writeln("VCVS, gain 5: node voltages", flat(voltage_gain.voltage), " VCVS current", flat(voltage_gain.vcvs_current));

  ckt::Net transconductor;
  transconductor.nodes = 3;
  transconductor.voltage_sources = {{1}, {0}, vec({1.0})};
  transconductor.resistors = {{2}, {0}, vec({1000.0})};
  transconductor.vccs = {{2}, {0}, {1}, {0}, vec({0.002})};
  const auto transconductance = ckt::op(ckt::make(transconductor));
  out.writeln("VCCS, 2 mS : node voltages", flat(transconductance.voltage), " VCCS current", flat(transconductance.vccs_current));
}
