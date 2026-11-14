<!-- generated from conduction.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Conduction and fins

Resistances of walls, tubes, spheres and films; the Biot number that says whether a body may be lumped; straight fins; and a layered wall divided into a network.

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

A composite wall against its network.

```cpp
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
```

Output

```text
layer resistances [0.14286, 2.00000, 0.03000] K/W per m^2, total 2.17286
heat flux through the network [11.50559] W/m^2  25 K / total = 11.50559
interface temperatures [20.00000, 18.35634, -4.65483, -5.00000]
```
### Example 2

A fin with three tip conditions.

```cpp
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
  // The three tip conditions for the same fin at two lengths. A convective tip adds the tip area; once the fin is long the three agree.
  const double h = 25.0, k = 200.0, p = 0.016, a = 1.6e-5;
  for (const double length : {0.05, 0.3}) {
    out.writeln("length", length, "m");
    for (const auto tip : {th::Tip::Adiabatic, th::Tip::Convective, th::Tip::Infinite}) {
      const auto f = th::fin(vec({h}), vec({k}), vec({p}), vec({a}), vec({length}), vec({100.0}), vec({20.0}), tip);
      out.writeln(tip == th::Tip::Adiabatic ? "  adiabatic " : tip == th::Tip::Convective ? "  convective" : "  infinite  ", "heat", flat(f.heat), "W  tip", flat(f.tip));
    }
  }
}
```

Output

```text
length 0.05000 m
  adiabatic  heat [1.45183] W  tip [88.94402]
  convective heat [1.47546] W  tip [88.55523]
  infinite   heat [2.86217] W  tip [20.00000]
length 0.30000 m
  adiabatic  heat [2.85519] W  tip [25.58370]
  convective heat [2.85534] W  tip [25.52211]
  infinite   heat [2.86217] W  tip [20.00000]
```
### Example 3

A slab diffusing in time.

```cpp
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
```

Output

```text
temperature profile at 10000 s (node 0 is the face at 100, node 21 the face at 0)
  node 0 100.00000
  node 1 97.49974
  node 5 77.49783
  node 10 52.49667
  node 15 27.49746
  node 20 2.49974
  node 21 -0.00000
series solution at the midplane (x = L/2): 49.99671  mesh node 10 and 11 average 49.99667
```
<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

conduction/wall
conduction/tube
conduction/shell
conduction/film
conduction/biot
conduction/Tip
conduction/FinResult
conduction/fin
conduction/Layers
conduction/Slab
conduction/mesh
```
<!-- apiref:toctree:end -->
