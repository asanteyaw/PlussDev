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
  // diffusion charge tau i makes a forward-biased diode an admittance g (1 + j w tau): its AC response against the formula
  const auto build = [](double volts) {
    ckt::Net net;
    net.nodes = 3;
    net.voltage_sources = {{1}, {0}, vec({volts})};
    net.resistors = {{1}, {2}, vec({1000.0})};
    net.diodes.positive = {2};
    net.diodes.negative = {0};
    net.diodes.saturation = vec({1e-14});
    net.diodes.transit_time = vec({1e-6});
    return ckt::make(net);
  };
  const auto bias = ckt::op(build(5.0));
  const double g = flat(bias.diode_current)[0] / 0.02585;
  out.writeln("diode conductance g (mS)", g * 1e3);
  for (double w : {1e3, 1e5, 1e6}) {
    const auto response = ckt::ac(build(1.0), vec({w}), bias).voltage.select(-1, 2);
    const std::complex<double> y(g, w * 1e-6 * g), expected = 1.0 / (1.0 + 1000.0 * y);
    out.writeln("omega", w, " |gain|", flat(response.abs())[0], " formula", std::abs(expected));
  }
}
