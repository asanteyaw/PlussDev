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
  net.nodes = 4;
  net.voltage_sources = {{1}, {0}, vec({0.5})};
  net.resistors = {{1, 2}, {2, 3}, vec({1000.0, 10000.0})};
  net.amps = {{3}, {0}, {0}, {2}};   // output 3, reference 0, positive input 0, negative input 2
  const auto result = ckt::op(ckt::make(net));
  out.writeln("node voltages", flat(result.voltage), " (node 2 is the virtual ground; node 3 the output)");
  out.writeln("op-amp current", flat(result.amp_current));
}
