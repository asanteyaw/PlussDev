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
  // a reverse-biased diode blocks, so its junction capacitance is all the node sees: a 5 V step through 1 kohm charges it
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.positive = {0};
  net.diodes.negative = {2};
  net.diodes.saturation = vec({1e-14});
  net.diodes.junction_capacitance = vec({1e-6});
  std::vector<double> time;
  for (int k = 0; k <= 30; ++k) time.push_back(k * 1e-4);
  const auto run = ckt::tran(ckt::make(net), vec(time));
  const std::vector<double> node = flat(run.voltage.select(1, 2));
  out.writeln("  ms   node     closed form");
  for (int ms = 0; ms <= 3; ++ms) out.writeln(ms, node[ms * 10], 5.0 * (1.0 - std::exp(-1.0 * ms)));
}
