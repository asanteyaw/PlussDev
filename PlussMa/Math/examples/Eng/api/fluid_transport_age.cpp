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

// a reservoir at 50 m feeding a junction through a 1 km pipe of 0.15 m; the pipe holds 17.67 m3 and the draw is 0.01 m3/s
static fluid::Net line(double demand = 0.01) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 5.0});
  net.nodes.demand = vec({0.0, demand});
  net.nodes.head = vec({50.0, 49.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({0.15});
  net.pipes.roughness = vec({1e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

static const double kPi = 3.14159265358979323846;
static const double kVolume = kPi * 0.15 * 0.15 / 4.0 * 1000.0; // m3
static const double kTransit = kVolume / 0.01;                   // s

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2).with_sep(""));
  const auto network = fluid::network(line());
  fluid::Schedule schedule;
  schedule.duration = 3.0 * 3600.0;
  schedule.step = 60.0;
  schedule.pattern_step = 24.0 * 3600.0;
  fluid::Quality age;
  age.mode = fluid::QualityMode::Age;
  fluid::RouteOptions route;
  route.step = 20.0;
  route.courant = 1.0;
  route.cells = 20;
  const auto r = fluid::transport(network, schedule, age, route);
  const auto time = flat(r.hydraulic.time), node = flat(r.node);
  for (std::size_t i = 0; i < time.size(); i += 24)
    out.writeln("t ", time[i], " s: age at the reservoir ", node[2 * i], " s, at the junction ", node[2 * i + 1], " s");
  out.writeln("age at the end ", node.back(), " s; pipe volume / flow = ", kTransit, " s");
}
