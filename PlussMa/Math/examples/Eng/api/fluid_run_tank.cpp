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
  fluid::Schedule schedule;
  schedule.duration = 12.0 * 3600.0;
  schedule.step = 1800.0;
  schedule.pattern_step = 3.0 * 3600.0;
  schedule.demand = pluss::series::make(std::vector<double>{0, 0.5, 0, 0, 1.0, 0, 0, 2.0, 0, 0, 3.0, 0}, {4, 3});
  const auto r = fluid::run(fluid::network(with_tank(400.0, false)), schedule);
  const auto time = flat(r.time), level = flat(r.tank_level);
  const auto into = r.flow.select(-1, 1).reshape({r.flow.sizes()[0]}).items<double>();
  const auto delivered = r.demand.select(-1, 1).reshape({r.demand.sizes()[0]}).items<double>();
  out.writeln("samples ", time.size(), ", every state converged ", r.converged.all().item<bool>());
  for (std::size_t i = 0; i < time.size(); i += 6)
    out.writeln("t ", time[i] / 3600.0, " h: demand ", delivered[i], ", flow into the tank ", into[i], ", level ", level[i]);
  double volume = 0.0;
  for (std::size_t i = 0; i + 1 < time.size(); ++i) volume += into[i] * (time[i + 1] - time[i]);
  out.writeln("volume into the tank ", volume, " m3, area * change of level ", 400.0 * (level.back() - level.front()), " m3");
}
