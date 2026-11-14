#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a slack and a load bus joined by a line of 0.01 + 0.1j per unit, with the given load
static power::Net two_bus(double active, double reactive) {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ};
  net.buses.p = vec({0.0, -active});
  net.buses.q = vec({0.0, -reactive});
  net.buses.vm = vec({1.0, 1.0});
  net.buses.va = vec({0.0, 0.0});
  net.lines.from = {0};
  net.lines.to = {1};
  net.lines.r = vec({0.01});
  net.lines.x = vec({0.1});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const power::Grid base = power::make(two_bus(0.5, 0.2));
  power::LoadPath path;
  path.p = vec({0.0, -1.5});
  path.q = vec({0.0, -0.6});
  const auto curve = power::trace(base, path);   // the default options stop at the target, factor 1

  const auto factor = flat(curve.factor), voltage = flat(curve.magnitude.select(-1, 1));
  out.writeln("factors: ", factor);
  out.writeln("shapes: voltage ", curve.magnitude.sizes(), ", power entering the line ", curve.from_power.sizes(), ", tangent ", curve.tangent.sizes());
  out.writeln("first point ", voltage.front(), "   ordinary power flow at the base load ", flat(power::ac(base).magnitude)[1]);
  out.writeln("last point  ", voltage.back(), "   ordinary power flow at the target load ", flat(power::ac(power::make(two_bus(1.5, 0.6))).magnitude)[1]);
  out.writeln("target reached: ", curve.reached.item<bool>());
}
