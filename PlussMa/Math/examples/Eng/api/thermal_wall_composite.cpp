#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // Layers in series add: brick, insulation, plaster. The network gives the same total.
  const pluss::Series k = vec({0.7, 0.04, 0.5});
  const pluss::Series thickness = vec({0.1, 0.08, 0.015});
  const pluss::Series layers = th::wall(k, thickness, vec({1.0}));
  const std::vector<double> r = flat(layers);
  const double total = r[0] + r[1] + r[2];
  out.writeln("layer resistances", r, "K/W per m^2, total", total);
  th::Net net;
  net.nodes = 4;
  net.links = {{0, 1, 2}, {1, 2, 3}, layers.reciprocal()};
  net.fixed = {{0, 3}, vec({20.0, -5.0})};
  const auto run = th::steady(net);
  out.writeln("heat flux through the network", flat(run.link_flow.select(0, 0)), "W/m^2  25 K / total =", 25.0 / total);
  out.writeln("interface temperatures", flat(run.temperature));
}
