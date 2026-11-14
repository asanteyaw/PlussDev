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
  out.print_defaults(PrintOptions{}.fixed(5));
  // C(v) = Cj0 (1 - v / Vj)^-m with m = 0.5: the reverse bias sets the capacitance, and with it the low-pass corner
  for (double bias : {-1.0, -3.0, -10.0}) {
    const auto at_bias = ckt::op(ckt::make(junction(bias, 1e-6)));
    const double c = 1e-6 / std::sqrt(1.0 - bias / 0.7);
    const double corner = 1.0 / (1000.0 * c);
    const auto response = ckt::ac(ckt::make(junction(1.0, 1e-6)), vec({corner}), at_bias);
    out.writeln("reverse bias", bias, " V  capacitance (uF)", c * 1e6, " corner (rad/s)", corner, " gain at the corner", flat(response.voltage.select(-1, 2).abs())[0], "(1 / sqrt 2 = 0.70711)");
  }
}
