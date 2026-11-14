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
  out.print_defaults(PrintOptions{}.fixed(4));
  // switching a forward junction: the stored charge (depletion plus diffusion) delays the rise of the node voltage
  const auto build = [](bool with_charge) {
    ckt::Net net;
    net.nodes = 3;
    net.voltage_sources = {{1}, {0}, vec({3.0})};
    net.resistors = {{1}, {2}, vec({1000.0})};
    net.diodes.positive = {2};
    net.diodes.negative = {0};
    net.diodes.saturation = vec({1e-14});
    if (with_charge) {
      net.diodes.junction_capacitance = vec({1e-9});
      net.diodes.junction_potential = vec({0.7});
      net.diodes.transit_time = vec({1e-7});
    }
    return ckt::make(net);
  };
  std::vector<double> time, wave;
  for (int k = 0; k <= 80; ++k) {
    time.push_back(k * 2e-8);
    wave.push_back(k < 20 ? 0.0 : 3.0);
  }
  ckt::Drive drive;
  drive.voltage = vec(wave).reshape({81, 1});
  const auto ideal = flat(ckt::tran(build(false), vec(time), drive).voltage.select(1, 2));
  const auto stored = flat(ckt::tran(build(true), vec(time), drive).voltage.select(1, 2));
  out.writeln("  t (us)   without charge   with charge");
  for (int k : {19, 20, 22, 25, 30, 40, 60, 80}) out.writeln(time[k] * 1e6, ideal[k], stored[k]);
}
