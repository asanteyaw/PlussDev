#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static ckt::Net common_source(double vdd, double vgs, double rd, double lambda = 0.0, std::int64_t polarity = 1) {
  // nodes: 1 supply, 2 drain, 3 gate drive; source grounded
  ckt::Net net;
  net.nodes = 4;
  net.voltage_sources = {{1, 3}, {0, 0}, vec({vdd, vgs})};
  net.resistors = {{1}, {2}, vec({rd})};
  net.mosfets.drain = {2};
  net.mosfets.gate = {3};
  net.mosfets.source = {0};
  net.mosfets.polarity = {polarity};
  net.mosfets.transconductance = vec({2e-3});
  net.mosfets.threshold = vec({1.0});
  if (lambda > 0.0) net.mosfets.channel_modulation = vec({lambda});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  // a large drain resistor pulls the transistor into the triode region: 5 vds^2 - 41 vds + 5 = 0
  const auto result = ckt::op(ckt::make(common_source(5.0, 5.0, 5000.0)));
  const double vds = flat(result.voltage)[2];
  out.writeln("drain voltage", vds, " closed form", (41.0 - std::sqrt(1681.0 - 100.0)) / 10.0);
  out.writeln("drain current", flat(result.drain_current)[0], " from the triode law", 2e-3 * (4.0 * vds - 0.5 * vds * vds));
  out.writeln("triode: the drain is below the overdrive of 4 V", vds < 4.0);
}
