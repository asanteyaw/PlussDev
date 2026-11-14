<!-- generated from wall.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::wall`

The conduction resistance of a plane wall.

## Signature

```cpp
Series thermal::wall(const Series& conductivity,
                     const Series& thickness,
                     const Series& area);
```

## What It Does

Returns `thickness / (conductivity * area)` in kelvin per watt, the resistance to heat that flows through a plane layer from one face to the other. Inputs broadcast, so a vector of thicknesses with a scalar conductivity gives a vector of resistances.

## Parameters

`conductivity`
: The thermal conductivity `k` in W/(m K), positive.

`thickness`
: The thickness in m, positive.

`area`
: The area normal to the flow in m^2, positive.

## Returns

The resistance in K/W with the broadcast shape of the inputs.

## Errors

Throws `pluss::Error` for an input that is not a real floating array, is not finite, or is not positive.

## Interpretation and Use

Layers in series add resistances; layers in parallel add conductances. For a network use the reciprocal as a `Links::conductance`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A concrete wall and a sweep of thickness.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static double one(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>()[0]; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // A concrete wall, 0.2 m thick, k = 1.4 W/(m K), 10 m^2: R = L / (k A).
  const pluss::Series r = th::wall(vec({1.4}), vec({0.2}), vec({10.0}));
  out.writeln("resistance", flat(r), "K/W  closed form", 0.2 / (1.4 * 10.0));
  out.writeln("heat flow for 20 K across the wall", 20.0 / one(r), "W");
  const pluss::Series sweep = th::wall(vec({1.4}), vec({0.1, 0.2, 0.4}), vec({10.0}));
  out.writeln("three thicknesses (broadcast)", flat(sweep), "K/W");
}
```

Output

```text
resistance [0.01429] K/W  closed form 0.01429
heat flow for 20 K across the wall 1400.00000 W
three thicknesses (broadcast) [0.00714, 0.01429, 0.02857] K/W
```
### Example 2

Layers in series, checked against a network.

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
### Example 3

Rejected inputs.

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
  report([&] { (void)th::wall(vec({0.0}), vec({0.1}), vec({1.0})); });     // zero conductivity
  report([&] { (void)th::wall(vec({1.4}), vec({-0.1}), vec({1.0})); });    // negative thickness
  report([&] { (void)th::wall(vec({1.4}), vec({0.1}), vec({std::nan("")})); }); // area not finite
  report([&] { (void)th::wall(pluss::series::make(std::vector<int>{1}), vec({0.1}), vec({1.0})); }); // not a real floating array
}
```

Output

```text
eng::thermal::wall: conductivity must be positive
eng::thermal::wall: thickness must be positive
eng::thermal::wall: area must be finite
eng::thermal::wall: conductivity must be a defined real floating array
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="tube.html">
    <span class="label">Next</span>
    <span class="title">tube</span>
  </a>
</div>
<!-- apiref:nav:end -->
