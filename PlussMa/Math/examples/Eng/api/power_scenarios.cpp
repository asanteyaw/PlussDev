#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

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

  const auto base = power::ac(power::make(three_bus()));
  power::Net outage = three_bus();
  outage.lines.active = {true, true, false};   // line 0 - 2 out of service
  const auto cut = power::ac(power::make(outage));
  out.writeln("all lines in:         load-bus voltage ", flat(base.magnitude)[2], ", slack output (p.u.) ", flat(base.injection.real())[0]);
  out.writeln("line 0-2 out:         load-bus voltage ", flat(cut.magnitude)[2], ", slack output (p.u.) ", flat(cut.injection.real())[0]);

  // three load levels in one call: a leading axis of three on the injections
  power::Net sweep;
  sweep.base = {100e6, 230e3};
  sweep.buses.kind = {power::BusKind::Slack, power::BusKind::PQ};
  sweep.buses.p = pluss::series::make(std::vector<double>{0, -0.2, 0, -0.5, 0, -1.0}).reshape({3, 2});
  sweep.buses.q = pluss::series::make(std::vector<double>{0, -0.08, 0, -0.2, 0, -0.4}).reshape({3, 2});
  sweep.buses.vm = vec({1.0, 1.0});
  sweep.buses.va = vec({0.0, 0.0});
  sweep.lines.from = {0};
  sweep.lines.to = {1};
  sweep.lines.r = vec({0.01});
  sweep.lines.x = vec({0.1});
  const power::Grid grid = power::make(sweep);
  const auto r = power::ac(grid);
  out.writeln("batch shape of the grid: ", grid.batch_shape(), "; magnitude shape ", r.magnitude.sizes());
  out.writeln("load-bus voltage at 0.2, 0.5 and 1.0 per unit of load: ", flat(r.magnitude.select(-1, 1)));
}
