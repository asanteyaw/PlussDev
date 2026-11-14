#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a ring of three buses: the slack at bus 0, a load of 0.5 per unit at bus 2, and bus 1 between them.
// Lines: 0 joins buses 0-1 (x = 0.1), line 1 joins 1-2 (x = 0.2), line 2 joins 0-2 (x = 0.3).
static power::Net ring() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PQ};
  net.buses.p = vec({0.0, 0.0, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.1});
  net.buses.vm = vec({1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.03});
  net.lines.x = vec({0.1, 0.2, 0.3});
  return net;
}


int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const power::Grid grid = power::make(ring());
  const auto factors = power::lodf(grid);
  const auto f = flat(factors.factor);
  out.writeln("LODF [monitored line][outage line], shape ", factors.factor.sizes(), ":");
  for (int m = 0; m < 3; ++m) out.writeln("  line ", m, ": ", f[3 * m], " ", f[3 * m + 1], " ", f[3 * m + 2]);
  out.writeln("valid outages: ", flat(factors.valid));

  const auto result = power::outages(grid);
  const auto after = flat(result.flow);
  out.writeln("base flows: ", flat(result.base_flow));
  for (int k = 0; k < 3; ++k) out.writeln("flows after losing line ", k, ": ", after[3 * k], " ", after[3 * k + 1], " ", after[3 * k + 2]);

  // the same outage by taking line 2 out of service and solving again
  power::Net direct = ring();
  direct.lines.active = {true, true, false};
  out.writeln("flows from a new DC solve without line 2: ", flat(power::dc(power::make(direct)).from_power));
}
