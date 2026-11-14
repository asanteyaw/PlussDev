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
  out.print_defaults(PrintOptions{}.fixed(5));
  // a swept gate voltage is a batch: cutoff, saturation and triode in one call
  const double gates[] = {0.5, 1.5, 2.0, 3.0, 4.0, 5.0};
  ckt::Net net = common_source(10.0, 0.0, 2000.0);
  net.voltage_sources.value = mat({10, 0.5, 10, 1.5, 10, 2.0, 10, 3.0, 10, 4.0, 10, 5.0}, {6, 2});
  const auto result = ckt::op(ckt::make(net));
  out.writeln("drain_current shape", result.drain_current.sizes(), " (gate value, MOSFET)");
  const std::vector<double> vd = flat(result.voltage.select(1, 2)), id = flat(result.drain_current);
  out.writeln("  Vgs    Vd      Id (mA)");
  for (int k = 0; k < 6; ++k) out.writeln(gates[k], vd[k], id[k] * 1e3);
}
