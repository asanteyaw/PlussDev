#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({1.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-14});
  const auto circuit = ckt::make(net);
  try {
    (void)ckt::ac(circuit, vec({1.0}));
  } catch (const pluss::Error &e) {
    out.writeln(e.message());
  }
  // an operating point of a different circuit is rejected as well
  ckt::Net other;
  other.nodes = 2;
  other.voltage_sources = {{1}, {0}, vec({1.0})};
  other.resistors = {{1}, {0}, vec({1.0})};
  try {
    (void)ckt::ac(circuit, vec({1.0}), ckt::op(ckt::make(other)));
  } catch (const pluss::Error &e) {
    out.writeln(e.message());
  }
}
