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
  // NPN common emitter: 10 V supply, 1 kohm collector resistor, 5 V base drive through 100 kohm
  ckt::Net net;
  net.nodes = 5;
  net.voltage_sources = {{1, 3}, {0, 0}, vec({10.0, 5.0})};
  net.resistors = {{1, 3}, {2, 4}, vec({1000.0, 100000.0})};
  net.transistors.collector = {2};
  net.transistors.base = {4};
  net.transistors.emitter = {0};
  net.transistors.polarity = {1};
  net.transistors.saturation = vec({1e-14});
  net.transistors.forward_gain = vec({100.0});

  const auto result = ckt::op(ckt::make(net));
  const std::vector<double> v = flat(result.voltage);
  const double ic = flat(result.collector_current)[0], ib = flat(result.base_current)[0];
  out.writeln("base voltage      ", v[4]);
  out.writeln("collector voltage ", v[2]);
  out.writeln("base current      ", ib);
  out.writeln("collector current ", ic);
  out.writeln("current gain Ic/Ib", ic / ib);
  out.writeln("emitter current   ", -(ic + ib));
}
