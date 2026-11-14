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
  // 5 V source, 1 kohm, silicon diode to ground
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-14});

  const auto result = ckt::op(ckt::make(net));
  const double vd = flat(result.voltage)[2];
  const double id = flat(result.diode_current)[0];
  out.writeln("diode voltage     ", vd);
  out.writeln("diode current     ", id);
  out.writeln("resistor current  ", (5.0 - vd) / 1000.0);
  out.writeln("Newton iterations ", result.iterations.items<std::int64_t>()[0]);
  out.writeln("residual below 1e-9", result.residual.item<double>() < 1e-9);
}
