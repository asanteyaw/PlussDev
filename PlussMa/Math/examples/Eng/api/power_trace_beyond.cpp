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
  const power::Grid grid = power::make(two_bus(0.0, 0.0));
  power::LoadPath path;
  path.p = vec({0.0, -1.0});
  path.q = vec({0.0, -0.4});
  power::TraceOptions beyond;
  beyond.target = 4.0;   // the nose is at 3.15975
  const auto curve = power::trace(grid, path, beyond);
  out.writeln("target 4.0: reached ", curve.reached.item<bool>(), ", the curve ends at the nose, load ", flat(curve.factor).back(), ", valid points ", curve.valid.all().item<bool>());

  power::Options newton;
  newton.max_iterations = 60;
  const auto direct = power::ac(power::make(two_bus(4.0, 1.6)), newton);
  out.writeln("ordinary power flow at that load: converged ", direct.converged.item<bool>(), ", residual ", direct.residual.item<double>(), " (no voltage exists there, and it does not say where the limit is)");
}
