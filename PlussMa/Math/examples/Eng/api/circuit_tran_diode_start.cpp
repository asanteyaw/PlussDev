#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({0.0})};
  net.diodes.positive = {1};
  net.diodes.negative = {2};
  net.diodes.saturation = vec({1e-12});
  net.resistors = {{2}, {0}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  ckt::Start charged;
  charged.capacitor_voltage = vec({3.0});
  std::vector<double> time;
  for (int k = 0; k <= 30; ++k) time.push_back(k * 1e-4);
  const auto run = ckt::tran(ckt::make(net), vec(time), {}, charged);
  const std::vector<double> load = flat(run.voltage.select(1, 2));
  out.writeln("  ms    load    closed form 3 exp(-t / RC)");
  for (int ms = 0; ms <= 3; ++ms) out.writeln(ms, load[ms * 10], 3.0 * std::exp(-1.0 * ms));
  out.writeln("diode current below 1e-9 A at every sample", run.diode_current.abs().amax().item<double>() < 1e-9);
}
