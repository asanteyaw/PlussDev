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
  // a tank holding 40 m2 x 3 m of water at 340 K cools towards 288 K; the reservoir is at the tank's head, so nothing flows
  auto net = with_tank(40.0, false);
  net.nodes.head = vec({23.0, 23.0, 23.0});
  net.nodes.demand = vec({0.0, 0.0, 0.0});
  const auto network = fluid::network(net);
  const double density = 998.2, cp = 4180.0, area = 40.0, level = 3.0, volume = area * level;
  fluid::Schedule schedule;
  schedule.duration = 3600.0;
  schedule.step = 600.0;
  schedule.pattern_step = 24.0 * 3600.0;
  fluid::RouteOptions route;
  route.step = 30.0;
  route.courant = 0.9;
  route.cells = 4;
  for (double ua : {0.0, 2000.0}) {
    fluid::Heat heat;
    heat.initial = vec({340.0, 340.0, 340.0});
    heat.ambient = vec({288.0, 288.0});
    heat.ua = vec({0.0, 0.0});
    heat.tank_ambient = vec({288.0});
    heat.tank_ua = vec({ua});
    const auto r = fluid::heat(network, schedule, heat, route);
    const double exact = 288.0 + 52.0 * std::exp(-ua * 3600.0 / (density * cp * volume));
    out.writeln("tank UA ", ua, " W/K: tank temperature after an hour ", flat(r.tank).back(), " K, exact ", exact, " K");
  }
}
