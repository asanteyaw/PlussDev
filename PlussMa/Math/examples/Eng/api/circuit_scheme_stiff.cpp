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
  // the RC step response with a step of ten time constants
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const auto circuit = ckt::make(net);
  const pluss::Series time = vec(grid(1e-2, 9));
  ckt::TranOptions gear;
  gear.scheme = ckt::Scheme::Gear2;
  out.writeln("Trapezoid", flat(ckt::tran(circuit, time).voltage.select(1, 2)));
  out.writeln("Gear2    ", flat(ckt::tran(circuit, time, {}, {}, gear).voltage.select(1, 2)));
}
