#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // a divider with a capacitor on its output node
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{2}, {0}, vec({10.0})};
  net.resistors = {{2, 1}, {1, 0}, vec({1000.0, 1000.0})};
  net.capacitors = {{1}, {0}, vec({1e-6})};
  const auto circuit = ckt::make(net);
  const auto operating = ckt::op(circuit);
  ckt::Start settled;
  settled.capacitor_voltage = operating.voltage.select(0, 1).reshape({1});
  out.writeln("from the operating point, node 1 at the first and last sample", flat(ckt::tran(circuit, vec(grid(1e-4, 21)), {}, settled).voltage.select(1, 1)).front(),
              flat(ckt::tran(circuit, vec(grid(1e-4, 21)), {}, settled).voltage.select(1, 1)).back());
  out.writeln("from rest, node 1 at the first and last sample              ", flat(ckt::tran(circuit, vec(grid(1e-4, 21))).voltage.select(1, 1)).front(),
              flat(ckt::tran(circuit, vec(grid(1e-4, 21))).voltage.select(1, 1)).back());
}
