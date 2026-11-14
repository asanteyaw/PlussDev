<!-- generated from Net.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `thermal::Net`

A lumped thermal network.

## Signature

```cpp
thermal::Net
```

## What It Does

Describes a network of nodes joined by conductances, with capacities, fixed temperatures and heat sources. It is the thermal counterpart of an electrical network, and it is solved by the same engine: temperature plays the part of voltage and heat flow the part of current.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `nodes` | The number of nodes, at least one. Nodes are numbered from zero. | required |
| `links` | A `Links`: the conductances between nodes. | empty |
| `stores` | A `Stores`: the thermal capacities of nodes. | empty |
| `fixed` | A `Fixed`: the nodes held at a prescribed temperature. | empty |
| `heats` | A `Heats`: the heat injected into nodes. | empty |

## Returns

A value; read by `steady` and `tran`.

## Errors

`steady` and `tran` throw `pluss::Error` for fewer than one node, and for the errors of the four groups; the message names the group.

## Interpretation and Use

Build the network once and pass it to `steady` for the long-run answer and to `tran` for the time response. Leading axes on any group form a batch; groups without a batch axis are shared by every member.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A room with a wall, ventilation and a heater.

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
  // A room: air (node 0) and a wall (node 1) against the outside (node 2) at -5 degrees; 1 kW heats the air.
  //   air -- 40 W/K -- wall -- 25 W/K -- outside,   plus 30 W/K of ventilation from air straight to outside
  th::Net net;
  net.nodes = 3;
  net.links = {{0, 1, 0}, {1, 2, 2}, vec({40.0, 25.0, 30.0})};
  net.stores = {{0, 1}, vec({5.0e4, 4.0e6})};
  net.fixed = {{2}, vec({-5.0})};
  net.heats = {{0}, vec({1000.0})};
  const auto run = th::steady(net);
  out.writeln("air, wall", flat(run.temperature));
  out.writeln("flows (air-wall, wall-outside, air-outside)", flat(run.link_flow));
  out.writeln("the outside absorbs", flat(run.fixed_flow), "W of the 1000 W");
  const double rise = 1000.0 / (30.0 + 1.0 / (1.0 / 40.0 + 1.0 / 25.0));
  out.writeln("closed form air rise over the outside", rise, " air", -5.0 + rise);
}
```

Output

```text
air, wall [17.03390, 8.55932, -5.00000]
flows (air-wall, wall-outside, air-outside) [338.98305, 338.98305, 661.01695]
the outside absorbs [-1000.00000] W of the 1000 W
closed form air rise over the outside 22.03390  air 17.03390
```
### Example 2

The same network seen in steady state.

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
  // Two conductances of 2 W/K in series between 100 and 20 degrees: nodes 0 and 2 are fixed, node 1 floats.
  th::Net net;
  net.nodes = 3;
  net.links = {{0, 1}, {1, 2}, vec({2.0, 2.0})};
  net.fixed = {{0, 2}, vec({100.0, 20.0})};
  const auto run = th::steady(net);
  out.writeln("temperature", flat(run.temperature));
  out.writeln("link flow", flat(run.link_flow), " (a to b)");
  out.writeln("boundary delivers", flat(run.fixed_flow), " (negative: absorbs)");
  out.writeln("closed form: midpoint", 60.0, " flow", 80.0 / (0.5 + 0.5));
  out.writeln("largest residual below 1e-9:", run.residual.abs().amax().item<double>() < 1e-9);
}
```

Output

```text
temperature [100.00000, 60.00000, 20.00000]
link flow [80.00000, 80.00000]  (a to b)
boundary delivers [80.00000, -80.00000]  (negative: absorbs)
closed form: midpoint 60.00000  flow 80.00000
largest residual below 1e-9: true
```
### Example 3

A lumped body cooling.

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
  // A body of 100 J/K, started at 100 degrees, cools through 0.5 W/K to surroundings at 20 degrees: T = 20 + 80 exp(-t G / C).
  th::Net net;
  net.nodes = 2;
  net.links = {{0}, {1}, vec({0.5})};
  net.stores = {{0}, vec({100.0})};
  net.fixed = {{1}, vec({20.0})};
  const auto run = th::tran(net, vec(grid(2.0, 101)), {}, th::Start{vec({100.0})});
  const std::vector<double> body = flat(run.temperature.select(1, 0));
  for (int s : {0, 25, 50, 100}) {
    out.writeln("t =", 2.0 * s, "s  T =", body[s], " closed form", 20.0 + 80.0 * std::exp(-2.0 * s * 0.5 / 100.0));
  }
  out.writeln("shape", run.temperature.sizes(), " (time, node)");
  out.writeln("time constant C / G =", 100.0 / 0.5, "s");
}
```

Output

```text
t = 0.00000 s  T = 100.00000  closed form 100.00000
t = 50.00000 s  T = 82.30393  closed form 82.30406
t = 100.00000 s  T = 68.52225  closed form 68.52245
t = 200.00000 s  T = 49.43011  closed form 49.43036
shape [101, 2]  (time, node)
time constant C / G = 200.00000 s
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Heats.html">
    <span class="label">Previous</span>
    <span class="title">Heats</span>
  </a>
  <a class="next-link" href="Result.html">
    <span class="label">Next</span>
    <span class="title">Result</span>
  </a>
</div>
<!-- apiref:nav:end -->
