#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static ckt::Net follower(double gamma, bool bulk_grounded) {
  // nodes: 1 drain supply, 2 gate drive, 3 source; the source resistor goes to ground
  ckt::Net net;
  net.nodes = 4;
  net.voltage_sources = {{1, 2}, {0, 0}, vec({10.0, 5.0})};
  net.resistors = {{3}, {0}, vec({1000.0})};
  net.mosfets.drain = {1};
  net.mosfets.gate = {2};
  net.mosfets.source = {3};
  net.mosfets.polarity = {1};
  if (bulk_grounded) net.mosfets.bulk = {0};
  net.mosfets.transconductance = vec({2e-3});
  net.mosfets.threshold = vec({1.0});
  if (gamma > 0.0) net.mosfets.body_effect = vec({gamma});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // a source follower: the source rises, the bulk stays at ground, and the raised source-bulk voltage lifts the threshold
  out.writeln("source voltage, body tied to the source        ", flat(ckt::op(ckt::make(follower(0.5, false))).voltage)[3]);
  out.writeln("source voltage, bulk grounded, gamma = 0        ", flat(ckt::op(ckt::make(follower(0.0, true))).voltage)[3]);
  out.writeln("source voltage, bulk grounded, gamma = 0.5      ", flat(ckt::op(ckt::make(follower(0.5, true))).voltage)[3]);
  out.writeln("source voltage, bulk grounded, gamma = 1.0      ", flat(ckt::op(ckt::make(follower(1.0, true))).voltage)[3]);
}
