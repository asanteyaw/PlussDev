#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  // gmin is a leakage conductance across every junction: it dominates a reverse-biased diode's current
  for (double gmin : {0.0, 1e-12, 1e-9}) {
    ckt::Net net;
    net.nodes = 3;
    net.voltage_sources = {{1}, {0}, vec({-5.0})};
    net.resistors = {{1}, {2}, vec({1000.0})};
    net.diodes.positive = {2};
    net.diodes.negative = {0};
    net.diodes.saturation = vec({1e-14});
    ckt::SolveOptions options;
    options.gmin = gmin;
    const auto result = ckt::op(ckt::make(net), options);
    out.writeln("gmin", gmin, " reverse diode current", flat(result.diode_current)[0]);
  }
}
