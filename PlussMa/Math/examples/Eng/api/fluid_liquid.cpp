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
  const double nu[3] = {1.0038e-6, 0.3646e-6, 1.0e-4};
  const double rho[3] = {998.2, 971.8, 870.0};
  const char *name[3] = {"water 20 C", "water 80 C", "oil       "};
  for (int i = 0; i < 3; ++i) {
    fluid::Liquid liquid;
    liquid.density = rho[i];
    liquid.kinematic_viscosity = nu[i];
    const auto network = fluid::network(one_pipe(0.02), liquid);
    const auto r = fluid::steady(network);
    out.writeln(name[i], ": density ", network.liquid().density, ", friction ", flat(r.friction_factor)[0], ", head loss ", flat(r.head_loss)[0], " m, pressure ", flat(r.pressure)[1] / 1000.0, " kPa");
  }
  const fluid::Liquid defaults;
  out.writeln("defaults: density ", defaults.density, ", viscosity ", defaults.kinematic_viscosity * 1e6, "e-6, gravity ", defaults.gravity);
}
