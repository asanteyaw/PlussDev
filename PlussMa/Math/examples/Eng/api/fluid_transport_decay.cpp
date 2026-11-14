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
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const auto network = fluid::network(line());
  const std::int64_t cells = 20;
  fluid::Schedule schedule;
  schedule.duration = 4.0 * 3600.0;
  schedule.step = kTransit / cells;
  schedule.pattern_step = 24.0 * 3600.0;
  fluid::RouteOptions route;
  route.step = kTransit / cells; // Courant number one: the front is carried exactly
  route.courant = 1.0;
  route.cells = cells;
  out.writeln("pipe volume ", kVolume, " m3, transit time ", kTransit, " s");
  for (double k : {0.0, 1e-4, 3e-4}) {
    fluid::Quality quality;
    quality.initial = vec({1.0, 0.0});
    quality.pipe_rate = vec({-k}); // a negative rate is a decay
    const auto r = fluid::transport(network, schedule, quality, route);
    out.writeln("k = ", k, ": junction ", flat(r.node).back(), ", exp(-k T) = ", std::exp(-k * kTransit), ", mass error ", flat(r.mass_error).back());
  }
  // the same decay with Courant 0.9 and 4 cells blurs the front
  fluid::RouteOptions coarse;
  coarse.step = 20.0;
  coarse.courant = 0.9;
  coarse.cells = 4;
  fluid::Quality quality;
  quality.initial = vec({1.0, 0.0});
  quality.pipe_rate = vec({-1e-4});
  fluid::Schedule longer = schedule;
  longer.step = 60.0;
  out.writeln("4 cells, Courant 0.9: junction ", flat(fluid::transport(network, longer, quality, coarse).node).back(), ", exact ", std::exp(-1e-4 * kTransit));
}
