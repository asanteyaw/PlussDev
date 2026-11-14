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
  power::TraceOptions options;
  options.stop = power::TraceStop::Nose;
  const auto curve = power::trace(grid, path, options);

  const auto factor = flat(curve.factor), voltage = flat(curve.magnitude.select(-1, 1));
  out.writeln(curve.points, " points; the nose was reached: ", curve.reached.item<bool>());
  out.writeln("load (p.u.)  load-bus voltage");
  for (std::size_t i = 0; i < factor.size(); i += 4) out.writeln(factor[i], "      ", voltage[i]);
  out.writeln("last point: load ", factor.back(), ", voltage ", voltage.back());
  out.writeln("closed form: load 3.15975, voltage 0.58482;  largest residual on the curve ", curve.residual.max().item<double>());
}
