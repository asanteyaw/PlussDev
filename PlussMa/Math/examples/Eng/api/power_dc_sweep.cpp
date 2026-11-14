#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

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
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  power::Net net = ring();
  net.buses.p = pluss::series::make(std::vector<double>{0, 0, -0.25, 0, 0, -0.5, 0, 0, -1.0}).reshape({3, 3});
  net.buses.q = pluss::series::make(std::vector<double>{0, 0, -0.05, 0, 0, -0.1, 0, 0, -0.2}).reshape({3, 3});
  const auto r = power::dc(power::make(net));
  const auto angle = flat(r.angle.select(-1, 2));
  out.writeln("load-bus angle for 0.25, 0.5 and 1.0 per unit (rad): ", angle);
  out.writeln("ratios to the first: ", angle[0] / angle[0], " ", angle[1] / angle[0], " ", angle[2] / angle[0]);
  out.writeln("shape of the angles: ", r.angle.sizes());
}
