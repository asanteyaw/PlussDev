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

  out.writeln("node voltages ", flat(result.voltage));
  out.writeln("source current", flat(result.source_current), " (negative: the source delivers 2.5 mA)");
  out.writeln("residual      ", result.residual.item<double>());
  out.writeln("axes of the empty groups: inductors", result.inductor_current.sizes(), " VCCS", result.vccs_current.sizes(), " VCVS", result.vcvs_current.sizes(), " amps", result.amp_current.sizes());
}
