#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // node 1 source, node 2 an RC low-pass, node 3 a VCVS of gain 2 with an inductor, nodes 4 and 5 an op-amp inverter with a transconductance draw
  ckt::Net net;
  net.nodes = 6;
  net.voltage_sources = {{1}, {0}, vec({1.0})};
  net.resistors = {{1, 3, 4, 5}, {2, 4, 5, 0}, vec({1000.0, 1000.0, 1000.0, 2000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  net.inductors = {{3}, {0}, vec({0.5})};
  net.vcvs = {{3}, {0}, {2}, {0}, vec({2.0})};
  net.vccs = {{5}, {0}, {2}, {0}, vec({1e-3})};
  net.amps = {{5}, {0}, {0}, {4}};
  const auto run = ckt::tran(ckt::make(net), vec(grid(1e-4, 11)));
  out.writeln("voltage         ", run.voltage.sizes());
  out.writeln("source_current  ", run.source_current.sizes());
  out.writeln("inductor_current", run.inductor_current.sizes());
  out.writeln("vccs_current    ", run.vccs_current.sizes());
  out.writeln("vcvs_current    ", run.vcvs_current.sizes());
  out.writeln("amp_current     ", run.amp_current.sizes());
  out.writeln("residual        ", run.residual.sizes());
}
