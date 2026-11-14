#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // A slab at 0 degrees whose inner face is held at 100: the face-to-centre profile diffuses in. Slab of 0.1 m, alpha = k / (rho c) = 1e-6 m^2/s.
  th::Layers layers;
  layers.cells = {20};
  layers.thickness = vec({0.1});
  layers.conductivity = vec({1.0});
  layers.density = vec({1000.0});
  layers.specific_heat = vec({1000.0});
  th::Slab w = th::mesh(layers, vec({1.0}).reshape({}));
  w.net.fixed = {{w.inner, w.outer}, vec({100.0, 0.0})};
  const auto run = th::tran(w.net, vec(grid(100.0, 101)));
  out.writeln("temperature profile at 10000 s (node 0 is the face at 100, node 21 the face at 0)");
  const std::vector<double> profile = flat(run.temperature.select(0, 100));
  for (int node : {0, 1, 5, 10, 15, 20, 21}) out.writeln("  node", node, profile[node]);
  double series = 0.0;
  for (int n = 1; n < 200; n += 2) series += std::sin(n * M_PI * 0.5) * std::exp(-n * n * M_PI * M_PI * 1e-6 * 1e4 / 0.01) / n;
  out.writeln("series solution at the midplane (x = L/2):", 100.0 * (0.5 - 2.0 / M_PI * series), " mesh node 10 and 11 average", 0.5 * (profile[10] + profile[11]));
}
