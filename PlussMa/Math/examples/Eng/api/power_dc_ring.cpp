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
  const auto r = power::dc(power::make(ring()));
  out.writeln("angles (rad):          ", flat(r.angle), "   by hand 0, -0.025, -0.075");
  out.writeln("flows from end (p.u.): ", flat(r.from_power), "   by hand 0.25 (0 to 1), 0.25 (1 to 2), 0.25 (0 to 2)");
  out.writeln("flows to end (p.u.):   ", flat(r.to_power), "   equal and opposite: the line is lossless");
  out.writeln("injections (p.u.):     ", flat(r.injection), "   the slack supplies the whole load");
  out.writeln("residual: ", flat(r.residual));
}
