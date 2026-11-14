<!-- generated from Slab.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::Slab`

A meshed wall as a thermal network.

## Signature

```cpp
thermal::Slab
```

## What It Does

A plane wall compiled into a `Net`. Node `0` is the inner surface, nodes `1 .. cells` are the cell centres from inside to outside and node `cells + 1` is the outer surface; the surface nodes have no capacity. The links between nodes include the half-cells next to the surfaces, so a fixed temperature or a film attached to a surface node acts on the true surface.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `net` | The network: `cells + 2` nodes, `cells + 1` links and one store per cell. |  |
| `inner` | The index of the inner surface node, `0`. |  |
| `outer` | The index of the outer surface node, `cells + 1`. |  |
| `cells` | The total number of cells. |  |

## Returns

A value returned by `mesh`.

## Errors

None: a slab is only constructed by a successful call.

## Interpretation and Use

Extend `net` before solving: fix the surface temperatures, or add nodes for the air on each side and films between them and the surfaces. The conductances of the net are per mesh, so `net.nodes` grows when you add nodes.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The nodes, conductances and capacities of a two-layer slab.

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
```

Output

```text
nodes 7  inner surface node 0  outer surface node 6  cells 5
conductances (W/K) [24.00000, 12.00000, 12.00000, 2.82353, 1.60000, 3.20000]
capacities (J/K)   [108000.00000, 108000.00000, 108000.00000, 1050.00000, 1050.00000]
total capacity 326100.00000  closed form 326100.00000
```
### Example 2

A steady flux through a slab.

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
```

Output

```text
1 + 1 cells 13.33333  3 + 2 cells 13.33333  20 + 10 cells 13.33333
closed form 20 / (0.2 / 0.8 + 0.05 / 0.04) = 13.33333 W/m^2
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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Layers.html">
    <span class="label">Previous</span>
    <span class="title">Layers</span>
  </a>
  <a class="next-link" href="mesh.html">
    <span class="label">Next</span>
    <span class="title">mesh</span>
  </a>
</div>
<!-- apiref:nav:end -->
