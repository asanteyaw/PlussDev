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

// reservoir 0 (80 m) feeds junction 1 through one pipe, and junction 2 through another unless second_pipe is false; a valve joins 1 -> 2
static fluid::Net with_valve(fluid::ValveKind kind, double setting, double K = 0.0, bool second_pipe = true) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 10.0, 5.0});
  net.nodes.demand = vec({0.0, 0.02, 0.02});
  net.nodes.head = vec({80.0, 55.0, 55.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({600.0});
  net.pipes.diameter = vec({0.20});
  net.pipes.roughness = vec({1e-4});
  net.pipes.minor = vec({0.0});
  if (second_pipe) {
    net.pipes.from = {0, 0};
    net.pipes.to = {1, 2};
    net.pipes.length = vec({600.0, 900.0});
    net.pipes.diameter = vec({0.20, 0.15});
    net.pipes.roughness = vec({1e-4, 1e-4});
    net.pipes.minor = vec({0.0, 0.0});
  }
  net.valves.kind = {kind};
  net.valves.from = {1};
  net.valves.to = {2};
  net.valves.diameter = vec({0.20});
  net.valves.minor = vec({K});
  net.valves.setting = vec({setting});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const double kPi = 3.14159265358979323846, g = 9.80665;
  for (double K : {0.0, 10.0, 100.0, 1000.0}) {
    const auto r = fluid::steady(fluid::network(with_valve(fluid::ValveKind::Throttle, 0.0, K)));
    const double q = flat(r.flow)[2], v = 4.0 * q / (kPi * 0.2 * 0.2);
    out.writeln("K = ", K, ": valve flow ", q, ", head across it ", flat(r.head_loss)[2], ", K v^2/2g = ", K * v * v / (2.0 * g));
  }
}
