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
  // small-signal gain of the common-source stage: -gm Rd with gm = kp vov = 4 mS
  const auto bias = ckt::op(ckt::make(common_source(10.0, 3.0, 500.0)));
  const auto response = ckt::ac(ckt::make(common_source(0.0, 1.0, 500.0)), vec({0.0, 1e3}), bias);
  const pluss::Series gain = response.voltage.select(-1, 2);
  out.writeln("gain at 0 rad/s", flat(gain.real())[0], " formula", -2e-3 * 2.0 * 500.0);
  // with channel-length modulation the output resistance ro = 1 / (lambda id) shunts the load
  const auto lam_bias = ckt::op(ckt::make(common_source(10.0, 3.0, 500.0, 0.05)));
  const auto lam = ckt::ac(ckt::make(common_source(0.0, 1.0, 500.0, 0.05)), vec({0.0}), lam_bias);
  out.writeln("gain with lambda 0.05", flat(lam.voltage.select(-1, 2).real())[0]);
}
