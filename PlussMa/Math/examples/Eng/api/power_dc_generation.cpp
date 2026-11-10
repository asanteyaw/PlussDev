#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::ma::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// three buses in a ring: a slack at bus 0, a load of 0.5 per unit at bus 2, and bus 1 in between.
// The two paths from bus 0 to bus 2, directly (x = 0.3) and through bus 1 (x = 0.1 + 0.2), have equal reactance.
static power::Net ring(double resistance_scale = 1.0) {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PQ};
  net.buses.p = vec({0.0, 0.0, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.1});
  net.buses.vm = vec({1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01 * resistance_scale, 0.02 * resistance_scale, 0.03 * resistance_scale});
  net.lines.x = vec({0.1, 0.2, 0.3});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  power::Net net = ring();
  net.buses.kind[1] = power::BusKind::PV;
  net.gens.bus = {1};
  net.gens.p = vec({0.3});
  net.gens.q = vec({0.0});
  const auto r = power::dc(power::make(net));
  out.writeln("injections (p.u.): ", flat(r.injection), "   the slack supplies 0.5 - 0.3 = 0.2");
  out.writeln("angles (rad):      ", flat(r.angle));
  auto flows = flat(r.from_power);
  for (double &flow : flows) flow += 0.0;   // turns a rounding-level -0 into 0
  out.writeln("flows (p.u.):      ", flows, "   lines 0-1, 1-2, 0-2");
}
