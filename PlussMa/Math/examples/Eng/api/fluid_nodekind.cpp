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

// reservoir (60 m) -> pipe -> junction 1 (demand 0.04) -> pipe -> tank; the tank bottom is at 20 m, its surface starts at 23 m
static fluid::Net with_tank(double area = 400.0, bool overflow = false, double initial_head = 23.0) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Tank};
  net.nodes.elevation = vec({0.0, 10.0, 20.0});
  net.nodes.demand = vec({0.0, 0.04, 0.0});
  net.nodes.head = vec({60.0, 50.0, initial_head});
  net.tanks.area = vec({area});
  net.tanks.min_level = vec({0.5});
  net.tanks.max_level = vec({6.0});
  net.tanks.overflow = {overflow};
  net.pipes.from = {0, 1};
  net.pipes.to = {1, 2};
  net.pipes.length = vec({800.0, 400.0});
  net.pipes.diameter = vec({0.15, 0.20});
  net.pipes.roughness = vec({1e-4, 1e-4});
  net.pipes.minor = vec({0.0, 0.0});
  return net;
}

static fluid::Net one_pipe_with_tank_entry() {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 5.0});
  net.nodes.demand = vec({0.0, 0.02});
  net.nodes.head = vec({50.0, 49.0});
  net.tanks.area = vec({400.0});
  net.tanks.min_level = vec({0.5});
  net.tanks.max_level = vec({6.0});
  net.tanks.overflow = {false};
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({0.30});
  net.pipes.roughness = vec({1e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto two = fluid::network(two_sources());
  out.writeln("two sources: ", two.reservoirs(), " reservoirs, ", two.junctions(), " junction, ", two.tanks(), " tanks");
  const auto tank = fluid::network(with_tank());
  out.writeln("with a tank: ", tank.reservoirs(), " reservoir, ", tank.junctions(), " junction, ", tank.tanks(), " tank");
  attempt("two junctions and no fixed-grade node", [] {
    auto net = two_sources();
    net.nodes.kind = {fluid::NodeKind::Junction, fluid::NodeKind::Junction, fluid::NodeKind::Junction};
    (void)fluid::network(net);
  });
  attempt("a tank node with no tank described", [] {
    auto net = with_tank();
    net.tanks = fluid::Tanks{};
    (void)fluid::network(net);
  });
  attempt("a tank described for a network that has none", [] {
    auto net = one_pipe_with_tank_entry();
    (void)fluid::network(net);
  });
}
