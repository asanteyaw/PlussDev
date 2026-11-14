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
  // The steady flux through the meshed wall is the series resistance of its layers, however it is divided.
  const auto flux = [&](std::vector<std::int64_t> cells) {
    th::Layers layers;
    layers.cells = cells;
    layers.thickness = vec({0.2, 0.05});
    layers.conductivity = vec({0.8, 0.04});
    layers.density = vec({1800.0, 30.0});
    layers.specific_heat = vec({900.0, 1400.0});
    th::Slab w = th::mesh(layers, vec({1.0}).reshape({}));
    w.net.fixed = {{w.inner, w.outer}, vec({20.0, 0.0})};
    return flat(th::steady(w.net).link_flow)[0];
  };
  out.writeln("1 + 1 cells", flux({1, 1}), " 3 + 2 cells", flux({3, 2}), " 20 + 10 cells", flux({20, 10}));
  out.writeln("closed form 20 / (0.2 / 0.8 + 0.05 / 0.04) =", 20.0 / (0.2 / 0.8 + 0.05 / 0.04), "W/m^2");
}
