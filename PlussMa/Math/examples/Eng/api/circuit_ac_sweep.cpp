#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({1.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({0.1e-6, 1e-6, 10e-6}).reshape({3, 1})};
  const auto result = ckt::ac(ckt::make(net), vec({100.0, 1000.0, 10000.0}));

  out.writeln("voltage shape", result.voltage.sizes(), " (capacitor value, frequency, node)");
  const pluss::Series gain = result.voltage.select(2, 2).abs();   // node 2, shape [3, 3]
  out.writeln("gain: rows are 0.1, 1 and 10 microfarad; columns 100, 1000 and 10000 rad/s");
  for (long row = 0; row < 3; ++row) out.writeln(flat(gain.select(0, row)));
}
