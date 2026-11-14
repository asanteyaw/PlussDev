<!-- generated from Layers.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::Layers`

The layers of a plane wall.

## Signature

```cpp
thermal::Layers
```

## What It Does

Describes a wall as layers from the inner face to the outer face. Each layer has a thickness, a conductivity, a density, a specific heat and a number of cells into which it is divided; `mesh` turns the description into a network.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `cells` | The number of equal cells in each layer, at least one each. One entry per layer. | required |
| `thickness` | The thickness of each layer in m, `[..., layers]`, positive. | required |
| `conductivity` | The conductivity of each layer in W/(m K), `[..., layers]`, positive. | required |
| `density` | The density of each layer in kg/m^3, `[..., layers]`, positive. | required |
| `specific_heat` | The specific heat of each layer in J/(kg K), `[..., layers]`, positive. | required |

## Returns

A value; read by `mesh`.

## Errors

`mesh` throws `pluss::Error` for arrays whose last axis does not have one value per layer, a cell count below one, or a value that is not positive or not finite.

## Interpretation and Use

More cells resolve the temperature profile; the steady heat flow does not depend on the number of cells, but a transient does. Put more cells where the temperature changes fastest, normally near the faces.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Two layers divided into five cells.

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

The steady flux for different divisions.

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

Rejected layer descriptions.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  th::Layers layers;
  layers.cells = {3, 2};
  layers.thickness = vec({0.2, 0.05});
  layers.conductivity = vec({0.8, 0.04});
  layers.density = vec({1800.0, 30.0});
  layers.specific_heat = vec({900.0, 1400.0});
  report([&] { (void)th::mesh(layers, vec({-1.0}).reshape({})); });   // negative area
  th::Layers few = layers;
  few.cells = {3};
  report([&] { (void)th::mesh(few, vec({1.0}).reshape({})); });       // one cell count for two layers
  th::Layers none = layers;
  none.cells = {3, 0};
  report([&] { (void)th::mesh(none, vec({1.0}).reshape({})); });      // a layer with no cells
  th::Layers thin = layers;
  thin.thickness = vec({0.2, 0.0});
  report([&] { (void)th::mesh(thin, vec({1.0}).reshape({})); });      // zero thickness
}
```

Output

```text
eng::thermal::mesh: area must be positive
eng::thermal::mesh: thickness, conductivity, density, specific_heat and cells must have one entry per layer
eng::thermal::mesh: every layer needs at least one cell
eng::thermal::mesh: thickness must be positive
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fin.html">
    <span class="label">Previous</span>
    <span class="title">fin</span>
  </a>
  <a class="next-link" href="Slab.html">
    <span class="label">Next</span>
    <span class="title">Slab</span>
  </a>
</div>
<!-- apiref:nav:end -->
