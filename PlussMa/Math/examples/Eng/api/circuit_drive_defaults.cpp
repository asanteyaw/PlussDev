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
  // one voltage source and one current source; only the voltage is driven, the current keeps its stored value
  ckt::Net net;
  net.nodes = 2;
  net.voltage_sources = {{1}, {0}, vec({0.0})};
  net.current_sources = {{1}, {0}, vec({0.001})};   // 1 mA, held for the whole run
  net.resistors = {{1}, {0}, vec({1000.0})};
  const std::vector<double> time = grid(1.0, 4);
  ckt::Drive voltage_only;
  voltage_only.voltage = vec({1.0, 2.0, 3.0, 4.0}).reshape({4, 1});
  const auto run = ckt::tran(ckt::make(net), vec(time), voltage_only);
  out.writeln("source current, with the drive      ", flat(run.source_current));
  const auto held = ckt::tran(ckt::make(net), vec(time));   // no drive at all: the stored 0 V is held
  out.writeln("source current, no drive (0 V held) ", flat(held.source_current));
  out.writeln("node 1 voltage, with the drive      ", flat(run.voltage.select(1, 1)));
}
