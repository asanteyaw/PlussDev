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

// a reservoir at 50 m feeding one junction through a 1 km pipe
static fluid::Net one_pipe(double demand = 0.02) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 5.0});
  net.nodes.demand = vec({0.0, demand});
  net.nodes.head = vec({50.0, 49.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({0.30});
  net.pipes.roughness = vec({1.0e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  fluid::Options pressure_driven;
  pressure_driven.demand_mode = fluid::DemandMode::Pressure;
  pressure_driven.min_pressure = 0.0;
  pressure_driven.full_pressure = 15.0;
  pressure_driven.demand_exponent = 0.5;
  for (double demand : {0.2, 0.6, 1.0}) {
    const auto fixed = fluid::steady(fluid::network(one_pipe(demand)));
    const auto driven = fluid::steady(fluid::network(one_pipe(demand)), pressure_driven);
    out.writeln("requested ", demand, ": Fixed delivers ", flat(fixed.demand)[1], " at pressure head ", flat(fixed.head)[1] - 5.0, "; Pressure delivers ", flat(driven.demand)[1], " at pressure head ", flat(driven.head)[1] - 5.0);
  }
  // an emitter: a leak that grows with the square root of the pressure head
  for (double c : {0.0, 0.002, 0.01}) {
    auto net = one_pipe(0.02);
    net.nodes.emitter = vec({0.0, c});
    const auto r = fluid::steady(fluid::network(net));
    const double p = flat(r.head)[1] - 5.0;
    out.writeln("emitter ", c, ": outflow ", flat(r.emitter)[1], ", C sqrt(p) = ", c * std::sqrt(p), ", pipe flow ", flat(r.flow)[0]);
  }
}
