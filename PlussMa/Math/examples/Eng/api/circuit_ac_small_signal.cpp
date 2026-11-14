#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  // bias the diode with 5 V behind 1 kohm, then excite the same circuit with a 1 V small signal
  const auto build = [](double volts) {
    ckt::Net net;
    net.nodes = 3;
    net.voltage_sources = {{1}, {0}, vec({volts})};
    net.resistors = {{1}, {2}, vec({1000.0})};
    net.capacitors = {{2}, {0}, vec({1e-7})};
    net.diodes.positive = {2};
    net.diodes.negative = {0};
    net.diodes.saturation = vec({1e-14});
    return ckt::make(net);
  };
  const auto bias = ckt::op(build(5.0));
  const double rd = 0.02585 / flat(bias.diode_current)[0];   // the diode's dynamic resistance, n Vt / Id
  const auto response = ckt::ac(build(1.0), vec({0.0, 1e4, 1e5}), bias);
  const auto gain = response.voltage.select(-1, 2).abs();
  out.writeln("dynamic resistance", rd);
  out.writeln("DC gain rd / (R + rd)", rd / (1000.0 + rd), " ac at omega = 0", flat(gain)[0]);
  out.writeln("gain at 1e4 and 1e5 rad/s", flat(gain)[1], flat(gain)[2]);
}
