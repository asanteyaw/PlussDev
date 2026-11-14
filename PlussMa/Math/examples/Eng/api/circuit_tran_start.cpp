#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // a discharging capacitor: the source is off and the capacitor starts at 4 V
  ckt::Net rc;
  rc.nodes = 3;
  rc.voltage_sources = {{1}, {0}, vec({0.0})};
  rc.resistors = {{1}, {2}, vec({1000.0})};
  rc.capacitors = {{2}, {0}, vec({1e-6})};
  ckt::Start charged;
  charged.capacitor_voltage = vec({4.0});
  const auto discharge = ckt::tran(ckt::make(rc), vec(grid(1e-4, 31)), {}, charged);
  const std::vector<double> v = flat(discharge.voltage.select(1, 2));
  out.writeln("capacitor at t = 0, tau, 3 tau", v[0], v[10], v[30], " closed form", 4.0, 4.0 * std::exp(-1.0), 4.0 * std::exp(-3.0));

  // a decaying inductor current: the source is off and the inductor starts at 0.25 A
  ckt::Net rl;
  rl.nodes = 3;
  rl.voltage_sources = {{1}, {0}, vec({0.0})};
  rl.resistors = {{1}, {2}, vec({10.0})};
  rl.inductors = {{2}, {0}, vec({0.01})};
  ckt::Start flowing;
  flowing.inductor_current = vec({0.25});
  const auto decay = ckt::tran(ckt::make(rl), vec(grid(1e-4, 31)), {}, flowing);
  const std::vector<double> i = flat(decay.inductor_current);
  out.writeln("inductor at t = 0, tau, 3 tau", i[0], i[10], i[30], " closed form", 0.25, 0.25 * std::exp(-1.0), 0.25 * std::exp(-3.0));
}
