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

// a reservoir (node 0) and four junctions joined by six pipes that form two loops
static fluid::Net looped() {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Junction,
                    fluid::NodeKind::Junction, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 10.0, 12.0, 8.0, 15.0});
  net.nodes.demand = vec({0.0, 0.010, 0.015, 0.012, 0.008});
  net.nodes.head = vec({60.0, 55.0, 55.0, 55.0, 55.0});
  net.pipes.from = {0, 1, 1, 2, 3, 2};
  net.pipes.to = {1, 2, 3, 4, 4, 3};
  net.pipes.length = vec({800.0, 500.0, 600.0, 450.0, 550.0, 300.0});
  net.pipes.diameter = vec({0.30, 0.20, 0.20, 0.15, 0.15, 0.10});
  net.pipes.roughness = vec({1e-4, 1e-4, 1e-4, 1e-4, 1e-4, 1e-4});
  net.pipes.minor = vec({0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  // three demand scenarios share one topology: only the demand array carries a leading axis
  auto net = looped();
  const double scale[3] = {0.5, 1.0, 2.0};
  std::vector<double> demand;
  for (double s : scale)
    for (double d : {0.0, 0.010, 0.015, 0.012, 0.008}) demand.push_back(d * s);
  net.nodes.demand = pluss::series::make(demand, {3, 5});
  const auto network = fluid::network(net);
  out.writeln("batch shape ", network.batch_shape(), ", dtype Float64: ", network.dtype() == pluss::ScalarType::Float64);
  const auto r = fluid::steady(network);
  out.writeln("head ", r.head.sizes(), ", flow ", r.flow.sizes(), ", converged ", r.converged.sizes());
  const auto head = flat(r.head);
  for (int s = 0; s < 3; ++s) out.writeln("demand x", scale[s], ": head at junction 4 ", head[s * 5 + 4]);
}
