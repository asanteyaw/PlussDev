#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static ckt::Net stage(std::int64_t polarity, double sign) {
  ckt::Net net;
  net.nodes = 5;
  net.voltage_sources = {{1, 3}, {0, 0}, vec({10.0 * sign, 5.0 * sign})};
  net.resistors = {{1, 3}, {2, 4}, vec({1000.0, 100000.0})};
  net.transistors.collector = {2};
  net.transistors.base = {4};
  net.transistors.emitter = {0};
  net.transistors.polarity = {polarity};
  net.transistors.saturation = vec({1e-14});
  net.transistors.forward_gain = vec({100.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  // a PNP on negated supplies is the mirror image of the NPN
  const auto npn = ckt::op(ckt::make(stage(1, 1.0)));
  const auto pnp = ckt::op(ckt::make(stage(-1, -1.0)));
  out.writeln("NPN collector voltage and current", flat(npn.voltage)[2], flat(npn.collector_current)[0]);
  out.writeln("PNP collector voltage and current", flat(pnp.voltage)[2], flat(pnp.collector_current)[0]);
}
