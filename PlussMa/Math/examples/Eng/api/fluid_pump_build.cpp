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

// a pump lifts water from a reservoir at 10 m to a reservoir at 40 m through a 500 m pipe
static fluid::Net transfer(double speed = 1.0) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Reservoir};
  net.nodes.elevation = vec({0.0, 5.0, 0.0});
  net.nodes.demand = vec({0.0, 0.0, 0.0});
  net.nodes.head = vec({10.0, 40.0, 40.0});
  net.pumps.from = {0};
  net.pumps.to = {1};
  net.pumps.shutoff_head = vec({60.0});
  net.pumps.resistance = vec({4000.0});
  net.pumps.exponent = vec({2.0});
  net.pumps.speed = vec({speed});
  net.pipes.from = {1};
  net.pipes.to = {2};
  net.pipes.length = vec({500.0});
  net.pipes.diameter = vec({0.20});
  net.pipes.roughness = vec({1e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const auto network = fluid::network(transfer(1.0));
  out.writeln("pipes ", network.pipes(), ", pumps ", network.pumps(), ", links ", network.links(), " (the pipe is link 0, the pump link 1)");
  const auto r = fluid::steady(network);
  const auto q = flat(r.flow), loss = flat(r.head_loss);
  out.writeln("flow ", q[0], " m3/s; pump gain ", -loss[1], " m, curve at that flow ", 60.0 - 4000.0 * q[1] * q[1], " m; pump velocity ", flat(r.velocity)[1]);
  out.writeln("junction head ", flat(r.head)[1], " m = 10 + gain");
}
