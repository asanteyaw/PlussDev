#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // gamma and the surface potential are device values: a batch of body-effect coefficients in one call
  ckt::Net net;
  net.nodes = 4;
  net.voltage_sources = {{1, 2}, {0, 0}, vec({10.0, 5.0})};
  net.resistors = {{3}, {0}, vec({1000.0})};
  net.mosfets.drain = {1};
  net.mosfets.gate = {2};
  net.mosfets.source = {3};
  net.mosfets.polarity = {1};
  net.mosfets.bulk = {0};
  net.mosfets.transconductance = vec({2e-3});
  net.mosfets.threshold = vec({1.0});
  net.mosfets.body_effect = vec({0.0, 0.3, 0.6, 0.9}).reshape({4, 1});
  const auto result = ckt::op(ckt::make(net));
  out.writeln("source voltage for gamma 0, 0.3, 0.6, 0.9", flat(result.voltage.select(1, 3)));
  out.writeln("drain current in mA                      ", flat(result.drain_current.mul(1e3)));
  net.mosfets.body_effect = vec({0.6});
  net.mosfets.surface_potential = vec({0.4, 0.6, 0.9}).reshape({3, 1});
  out.writeln("source voltage for phi 0.4, 0.6, 0.9      ", flat(ckt::op(ckt::make(net)).voltage.select(1, 3)));
}
