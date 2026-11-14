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
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({1.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const pluss::Series omega = vec({0.0, 1.0, 1000.0, 100000.0});

  const auto result = ckt::ac(ckt::make(net), omega);
  const pluss::Series output = result.voltage.select(1, 2);   // the capacitor voltage, node 2
  out.writeln("voltage shape", result.voltage.sizes(), " element type", result.voltage.dtype().name);
  out.writeln("gain         ", flat(output.abs()));
  out.writeln("phase (deg)  ", flat(output.angle() * (180.0 / M_PI)));
  out.writeln("analytic gain", 1.0, 1.0 / std::sqrt(1.0 + 1e-6), 1.0 / std::sqrt(2.0), 1.0 / std::sqrt(1.0 + 1e4));
  out.writeln("source current magnitude", flat(result.source_current.abs()));
}
