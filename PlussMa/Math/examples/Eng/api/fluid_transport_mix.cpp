#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <functional>
#include <iostream>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static void attempt(const char *label, const std::function<void()> &call) {
  try {
    call();
    std::cout << label << ": accepted\n";
  } catch (const std::exception &e) {
    std::cout << label << ": " << e.what() << "\n";
  }
}

// two reservoirs feed junction 2 through pipes of different length
static fluid::Net two_sources() {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 0.0, 5.0});
  net.nodes.demand = vec({0.0, 0.0, 0.02});
  net.nodes.head = vec({50.0, 48.0, 40.0});
  net.pipes.from = {0, 1};
  net.pipes.to = {2, 2};
  net.pipes.length = vec({600.0, 300.0});
  net.pipes.diameter = vec({0.15, 0.15});
  net.pipes.roughness = vec({1e-4, 1e-4});
  net.pipes.minor = vec({0.0, 0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const auto network = fluid::network(two_sources());
  const auto flow = flat(fluid::steady(network).flow);
  fluid::Schedule schedule;
  schedule.duration = 1800.0;
  schedule.step = 120.0;
  schedule.pattern_step = 24.0 * 3600.0;
  fluid::Quality quality;
  quality.initial = vec({1.0, 0.0, 0.0});
  fluid::RouteOptions route;
  route.step = 10.0;
  route.courant = 0.9;
  route.cells = 20;
  const auto r = fluid::transport(network, schedule, quality, route);
  out.writeln("shapes: node ", r.node.sizes(), ", link ", r.link.sizes(), ", tank ", r.tank.sizes(), ", mass ", r.mass.sizes());
  const auto time = flat(r.hydraulic.time), node = flat(r.node), mass = flat(r.mass), err = flat(r.mass_error);
  for (std::size_t i = 0; i < time.size(); i += 3)
    out.writeln("t ", time[i], " s: junction ", node[3 * i + 2], ", stored mass ", mass[i], ", mass error ", err[i]);
  out.writeln("final ", node.back(), ", flow-weighted average ", flow[0] / (flow[0] + flow[1]));
}
