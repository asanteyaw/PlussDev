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
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  const auto base = [] {
    ckt::Net net;
    net.nodes = 3;
    net.voltage_sources = {{1}, {0}, vec({5.0})};
    net.resistors = {{1}, {2}, vec({1000.0})};
    net.diodes.positive = {2};
    net.diodes.negative = {0};
    net.diodes.saturation = vec({1e-14});
    net.diodes.junction_capacitance = vec({1e-9});
    return net;
  };
  auto potential = base();
  potential.diodes.junction_potential = vec({-0.7});
  report([&] { (void)ckt::make(potential); });
  auto grading = base();
  grading.diodes.junction_potential = vec({0.7});
  grading.diodes.grading = vec({1.0});
  report([&] { (void)ckt::make(grading); });
  auto transit = base();
  transit.diodes.transit_time = vec({-1e-9});
  report([&] { (void)ckt::make(transit); });
  auto capacitance = base();
  capacitance.diodes.junction_capacitance = vec({-1e-9});
  report([&] { (void)ckt::make(capacitance); });
}
