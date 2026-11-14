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
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.inductors = {{1}, {2}, vec({0.01})};
  net.resistors = {{2}, {0}, vec({100.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const auto result = ckt::op(ckt::make(net));
  out.writeln("node voltages   ", flat(result.voltage));
  out.writeln("inductor current", flat(result.inductor_current));
  out.writeln("source current  ", flat(result.source_current));
}
