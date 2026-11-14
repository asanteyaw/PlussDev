#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }


int main() {
  OutputEngine out;
  // an ideal source straight across a diode: nothing limits the current
  ckt::Net net;
  net.nodes = 2;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.diodes.positive = {1};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-14});
  const auto circuit = ckt::make(net);
  try {
    (void)ckt::op(circuit);
  } catch (const pluss::Error &e) {
    out.writeln(e.message());
  }
  // a series resistance makes it an ordinary bias point; or accept the model's answer by raising max_current
  ckt::SolveOptions allowed;
  allowed.max_current = 1e80;
  out.writeln("with max_current raised, the diode current", flat(ckt::op(circuit, allowed).diode_current)[0]);
}
