#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({10.0})};
  net.resistors = {{1, 2}, {2, 0}, vec({1000, 1000, 1000, 2000, 1000, 4000}).reshape({3, 2})};
  const auto result = ckt::op(ckt::make(net));
  out.writeln("voltage shape", result.voltage.sizes(), " source_current shape", result.source_current.sizes(), " residual shape", result.residual.sizes());
  out.writeln("middle node over the sweep", result.voltage.select(1, 2).items<double>());
}
