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
  net.nodes = 4;
  net.voltage_sources = {{1}, {0}, vec({1.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {3}, vec({1e-6})};
  net.amps = {{3}, {0}, {0}, {2}};
  const auto result = ckt::ac(ckt::make(net), vec({100.0, 1000.0, 10000.0}));
  const pluss::Series output = result.voltage.select(1, 3);
  out.writeln("gain       ", flat(output.abs()), " expected 10, 1, 0.1");
  out.writeln("phase (deg)", flat(output.angle() * (180.0 / M_PI)));
}
