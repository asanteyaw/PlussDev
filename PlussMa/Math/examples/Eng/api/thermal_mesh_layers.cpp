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
  // Two layers: 0.2 m of brick in 3 cells and 0.05 m of insulation in 2 cells, on 1 m^2.
  th::Layers layers;
  layers.cells = {3, 2};
  layers.thickness = vec({0.2, 0.05});
  layers.conductivity = vec({0.8, 0.04});
  layers.density = vec({1800.0, 30.0});
  layers.specific_heat = vec({900.0, 1400.0});
  const th::Slab w = th::mesh(layers, vec({1.0}).reshape({}));
  out.writeln("nodes", w.net.nodes, " inner surface node", w.inner, " outer surface node", w.outer, " cells", w.cells);
  out.writeln("conductances (W/K)", flat(w.net.links.conductance));
  out.writeln("capacities (J/K)  ", flat(w.net.stores.capacity));
  out.writeln("total capacity", w.net.stores.capacity.sum().item<double>(), " closed form", 1800.0 * 900.0 * 0.2 + 30.0 * 1400.0 * 0.05);
}
