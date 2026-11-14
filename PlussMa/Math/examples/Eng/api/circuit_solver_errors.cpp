#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-14});
  const auto circuit = ckt::make(net);
  ckt::SolveOptions few;
  few.max_iterations = 3;
  report([&] { (void)ckt::op(circuit, few); });
  ckt::SolveOptions cold;
  cold.thermal_voltage = 0.0;
  report([&] { (void)ckt::op(circuit, cold); });
  ckt::SolveOptions loose;
  loose.tolerance = -1.0;
  report([&] { (void)ckt::op(circuit, loose); });
  ckt::SolveOptions leaky;
  leaky.gmin = -1e-12;
  report([&] { (void)ckt::op(circuit, leaky); });
  ckt::SolveOptions none;
  none.max_iterations = 0;
  report([&] { (void)ckt::op(circuit, none); });
}
