#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({10.0})};
  net.resistors = {{1, 2}, {2, 0}, vec({1000.0, 3000.0})};
  const auto result = ckt::op(ckt::make(net));
  const std::vector<double> v = flat(result.voltage);

  const double dissipated = (v[1] - v[2]) * (v[1] - v[2]) / 1000.0 + v[2] * v[2] / 3000.0;
  const double delivered = -10.0 * flat(result.source_current)[0];
  out.writeln("power dissipated in the resistors", dissipated);
  out.writeln("power delivered by the source    ", delivered);
}
