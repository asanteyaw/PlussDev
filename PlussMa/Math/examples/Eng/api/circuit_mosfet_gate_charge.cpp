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
  // the gate capacitance is a capacitor: a 5 V step through 1 kohm charges 1 uF with tau = 1 ms
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.mosfets.drain = {1};
  net.mosfets.gate = {2};
  net.mosfets.source = {0};
  net.mosfets.polarity = {1};
  net.mosfets.transconductance = vec({2e-3});
  net.mosfets.threshold = vec({1.0});
  net.mosfets.gate_source_capacitance = vec({1e-6});
  std::vector<double> time;
  for (int k = 0; k <= 30; ++k) time.push_back(k * 1e-4);
  const auto run = ckt::tran(ckt::make(net), vec(time));
  const std::vector<double> gate = flat(run.voltage.select(1, 2));
  out.writeln("  ms   gate     closed form");
  for (int ms = 0; ms <= 3; ++ms) out.writeln(ms, gate[ms * 10], 5.0 * (1.0 - std::exp(-1.0 * ms)));
  out.writeln("drain_current shape", run.drain_current.sizes(), " (time sample, MOSFET)");
}
