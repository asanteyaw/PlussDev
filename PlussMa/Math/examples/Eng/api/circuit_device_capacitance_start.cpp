#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // capacitors in parallel must start at the same voltage: the disagreement is reported
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({0.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  net.diodes.positive = {0};
  net.diodes.negative = {2};
  net.diodes.saturation = vec({1e-14});
  net.diodes.junction_capacitance = vec({2e-6});
  const auto circuit = ckt::make(net);
  std::vector<double> time;
  for (int k = 0; k <= 20; ++k) time.push_back(k * 1e-4);
  ckt::Start agree;
  agree.capacitor_voltage = vec({2.0, -2.0});   // node 2 starts 2 V above ground: the net capacitor (node 2 to ground) holds 2 V, the diode capacitor (ground to node 2) -2 V
  const auto run = ckt::tran(circuit, vec(time), {}, agree);
  out.writeln("node 2 starts at", flat(run.voltage.select(1, 2))[0], " and relaxes to", flat(run.voltage.select(1, 2)).back());
  ckt::Start disagree;
  disagree.capacitor_voltage = vec({1.0, 2.0});
  try {
    (void)ckt::tran(circuit, vec(time), {}, disagree);
  } catch (const pluss::Error &e) {
    out.writeln(e.message());
  }
}
