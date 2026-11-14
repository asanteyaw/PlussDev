#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static ckt::Net junction(double source, double cj0) {
  // a source behind 1 kohm drives a diode (anode at node 2, cathode at ground) with a voltage-dependent junction capacitance
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({source})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-14});
  net.diodes.junction_capacitance = vec({cj0});
  net.diodes.junction_potential = vec({0.7});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  // a small step on a reverse-biased junction charges C(bias) through 1 kohm: tau = R Cj0 / sqrt(1 - v / Vj)
  const double c = 1e-6 / std::sqrt(1.0 + 3.0 / 0.7), tau = 1000.0 * c;
  std::vector<double> time, wave;
  for (int k = 0; k <= 40; ++k) {
    time.push_back(k * tau / 10.0);
    wave.push_back(k == 0 ? -3.0 : -2.999);
  }
  ckt::Drive step;
  step.voltage = vec(wave).reshape({41, 1});
  const auto run = ckt::tran(ckt::make(junction(-3.0, 1e-6)), vec(time), step);
  const std::vector<double> node = flat(run.voltage.select(1, 2));
  out.writeln("time constant (ms)", tau * 1e3);
  out.writeln("  t / tau   node - (-3 V) in mV   closed form");
  for (int k : {10, 20, 30, 40}) out.writeln(k / 10.0, (node[k] + 3.0) * 1e3, 1.0 * (1.0 - std::exp(-k / 10.0)));
}
