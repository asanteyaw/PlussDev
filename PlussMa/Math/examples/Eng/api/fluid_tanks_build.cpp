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

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto network = fluid::network(with_tank(400.0, false));
  out.writeln("nodes ", network.nodes(), ", junctions ", network.junctions(), ", reservoirs ", network.reservoirs(), ", tanks ", network.tanks());
  const auto r = fluid::steady(network);
  out.writeln("a steady solve holds the tank at its initial head: ", flat(r.head)[2], " m; flow into it ", flat(r.flow)[1], " m3/s");
  fluid::Schedule schedule;
  schedule.duration = 8.0 * 3600.0;
  schedule.step = 3600.0;
  schedule.pattern_step = 24.0 * 3600.0;
  for (bool overflow : {false, true}) {
    const auto run = fluid::run(fluid::network(with_tank(40.0, overflow)), schedule);
    const auto into = run.flow.select(-1, 1).reshape({run.flow.sizes()[0]}).items<double>();
    out.writeln("area 40 m2, overflow ", overflow, ": level after 8 h ", flat(run.tank_level).back(), " m (max 6 m), flow into the tank ", into.back(), ", spill ", flat(run.spill).back());
  }
}
