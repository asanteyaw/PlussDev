#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// the three-bus network of chapter 1
static power::Net three_bus() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.2, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.2});
  net.buses.vm = vec({1.04, 1.02, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.gens.bus = {1};
  net.gens.p = vec({0.6});
  net.gens.q = vec({0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.015});
  net.lines.x = vec({0.1, 0.2, 0.25});
  net.lines.charge = vec({0.02, 0.05, 0.04});
  return net;
}


int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const power::Grid grid = power::make(three_bus());
  power::LoadPath path;
  path.p = vec({0.0, 0.4, -1.0});    // bus 1 keeps its net injection of 0.4; the load at bus 2 grows to 1.0
  path.q = vec({0.0, 0.0, -0.4});
  power::TraceOptions options;
  options.stop = power::TraceStop::Nose;
  const auto curve = power::trace(grid, path, options);
  out.writeln("trace: nose reached ", curve.reached.item<bool>(), " at loading factor ", flat(curve.factor).back(), " after ", curve.points, " points, load-bus voltage ", flat(curve.magnitude.select(-1, 2)).back());

  // bisection on the ordinary power flow: the load at bus 2 is 0.5 + factor * 0.5 and 0.2 + factor * 0.2
  double low = 0.0, high = 20.0;
  const auto converges = [&](double factor) {
    power::Net net = three_bus();
    net.buses.p = vec({0.0, -0.2, -0.5 - 0.5 * factor});
    net.buses.q = vec({0.0, 0.0, -0.2 - 0.2 * factor});
    power::Options newton;
    newton.max_iterations = 60;
    return power::ac(power::make(net), newton).converged.item<bool>();
  };
  for (int i = 0; i < 40; ++i) (converges(0.5 * (low + high)) ? low : high) = 0.5 * (low + high);
  out.writeln("largest factor at which the ordinary power flow converges: ", low);
}
