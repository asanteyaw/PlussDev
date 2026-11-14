#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-14});
  const auto circuit = ckt::make(net);
  // a looser tolerance stops sooner; the answer changes only in the last digits
  for (double tolerance : {1e-3, 1e-6, 1e-10}) {
    ckt::SolveOptions options;
    options.tolerance = tolerance;
    const auto result = ckt::op(circuit, options);
    out.writeln("tolerance 1e", static_cast<int>(std::lround(std::log10(tolerance))), " iterations", result.iterations.items<std::int64_t>()[0], " diode voltage", flat(result.voltage)[2]);
  }
}
