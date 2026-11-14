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
  // channel-length modulation lambda makes the saturation current grow with the drain voltage: 10 - vd = 2 (1 + lambda vd)
  const auto flat_model = ckt::op(ckt::make(common_source(10.0, 3.0, 500.0)));
  const auto modulated = ckt::op(ckt::make(common_source(10.0, 3.0, 500.0, 0.05)));
  out.writeln("lambda 0   drain voltage", flat(flat_model.voltage)[2], " current", flat(flat_model.drain_current)[0]);
  out.writeln("lambda 0.05 drain voltage", flat(modulated.voltage)[2], " current", flat(modulated.drain_current)[0], " closed form", 8.0 / 1.1);
}
