<!-- generated from networks_and_steady_state.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Networks and steady state

[Back to Thermal 1: Conduction](../thermal_01_conduction.md)

You have a few bodies that exchange heat with each other and with their surroundings, and you want to know where each settles, how much heat flows through each connection, and how much the surroundings supply. This page builds such a network, solves its steady state, and reads the answers.

## The mental model

A thermal network is an electrical network with different names. A node has a temperature (the voltage), a connection between two nodes has a conductance in watts per kelvin (the reciprocal of a resistance), and the heat that flows is the current. Heat flows from the warmer node to the cooler one at `G` times the temperature difference. At every node that is not held, the heat that arrives equals the heat that leaves, plus whatever a heater injects: that is the whole of steady state. Because the equations are the circuit equations, the solve is done by the circuit engine, and everything it knows (batches, differentiation, dtype) comes with it.

A network is a `Net`: a count of nodes numbered from zero, `Links` between pairs, `Fixed` temperatures at boundary nodes, `Heats` injected into nodes, and `Stores` that matter only for time (next page). A steady state needs every node to reach a fixed node through links, since otherwise there is nothing to hold its temperature.

## A chain between two temperatures

Three nodes in a row, each link `2 W/K`, with the outer nodes held at 100 and 20 degrees. The middle node floats. The program reports the temperatures, the heat through each link, and the heat the two boundaries deliver.

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
The middle node settles at 60 degrees, halfway between the boundaries because the two links are equal. Each link carries `80 W`, which is `80 K` over the series resistance of `0.5 + 0.5 K/W`. The boundary at 100 degrees delivers 80 W into the network and the boundary at 20 degrees absorbs 80 W, which is why its `fixed_flow` is negative. The residual confirms the heat balance to rounding error.

## Injecting heat

A heater, an electronic part or solar gain is a heat source at a node. Putting 30 W into the middle node:

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
  // The same chain with 30 W injected into the middle node: both boundaries absorb it.
  th::Net net;
  net.nodes = 3;
  net.links = {{0, 1}, {1, 2}, vec({2.0, 2.0})};
  net.fixed = {{0, 2}, vec({100.0, 20.0})};
  net.heats = {{1}, vec({30.0})};
  const auto run = th::steady(net);
  out.writeln("temperature", flat(run.temperature));
  out.writeln("link flow", flat(run.link_flow));
  out.writeln("boundary delivers", flat(run.fixed_flow));
  out.writeln("injected + delivered =", 30.0 + flat(run.fixed_flow)[0] + flat(run.fixed_flow)[1], " (energy balance)");
}
```

Output

```text
temperature [100.00000, 67.50000, 20.00000]
link flow [65.00000, 95.00000]
boundary delivers [65.00000, -95.00000]
injected + delivered = 0.00000  (energy balance)
```
The node warms by `30 W / (2 + 2 W/K) = 7.5 K`, and the two boundaries share the 30 W in proportion to their conductances: the boundary at 100 degrees now delivers less (65 W) and the one at 20 degrees absorbs more (95 W). The last line is the energy balance you should check in any network: injected heat plus the heat delivered by the boundaries is zero in steady state.

## A room

A real model has more than a chain. Here air (node 0) is heated by 1 kW, loses heat through a wall (node 1) to the outside (node 2) at -5 degrees, and also through ventilation, which is a second path from the air straight to the outside.

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
The wall and the ventilation are in parallel, so their conductances add: the wall path is two links in series (`40` and `25 W/K`, which make `15.4 W/K`) and the ventilation is `30 W/K`. The air settles `22.0 K` above the outside. The closed-form line shows the same number from the series and parallel rules, which is the cross-check to make whenever a network is small enough.

## Building conductances from the physics

You rarely know a conductance; you know thicknesses and conductivities. The resistance functions give `K/W` and a network wants `W/K`, so a conductance is `resistance.reciprocal()`. Three layers of a wall, per square metre:

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
The layer resistances add to `2.17 K/W` per square metre and 25 K across them drives `11.5 W` through each square metre. The insulation, with 92 % of the resistance, takes 92 % of the temperature drop, which you can see in the interface temperatures. The network and the hand calculation agree because they are the same mathematics.

## Sweeping a design

Leading axes of any group are a batch. Give the second link three conductances and one call solves three networks:

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // A leading axis sweeps the conductance of the second link; one call solves every case.
  th::Net net;
  net.nodes = 3;
  net.links = {{0, 1}, {1, 2}, mat({2.0, 0.5, 2.0, 2.0, 2.0, 8.0}, {3, 2})};
  net.fixed = {{0, 2}, vec({100.0, 20.0})};
  const auto run = th::steady(net);
  out.writeln("temperature shape", run.temperature.sizes(), " (case, node)");
  out.writeln("middle node per case", flat(run.temperature.select(1, 1)));
  out.writeln("heat flow per case", flat(run.link_flow.select(1, 0)));
  out.writeln("closed form", 80.0 / (1.0 / 2.0 + 1.0 / 0.5), 80.0 / (1.0 / 2.0 + 1.0 / 2.0), 80.0 / (1.0 / 2.0 + 1.0 / 8.0));
}
```

Output

```text
temperature shape [3, 3]  (case, node)
middle node per case [84.00000, 60.00000, 36.00000]
heat flow per case [32.00000, 80.00000, 128.00000]
closed form 32.00000 80.00000 128.00000
```
The first axis of every result is the case, the second the node. The three middle temperatures and heat flows match the closed form on the last line. A batch costs almost nothing more than one solve, so sweep rather than loop.

## Decisions

- **Where a node goes:** wherever you need a temperature, or wherever the temperature changes (a surface, a mass, the air in a room). A node with no store is instantaneous and is right for a thin surface.
- **Fixed temperature or heat:** use `Fixed` when the surroundings are so large that heat does not move them (outdoor air, the ground, a thermostat); use `Heats` when you know the power and want the temperature.
- **One temperature scale:** conduction depends on differences only, so degrees Celsius or kelvin both work, but never both in one network.
- **A parameter that varies:** give it a leading axis rather than writing a loop.

## Pitfalls

A network that cannot be solved is refused with the node named. Each call below breaks one rule: a node with no link, nothing fixed, a node index outside the network, a link from a node to itself, and a conductance that is not positive.

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
  th::Net net;
  net.nodes = 3;
  net.links = {{0}, {1}, vec({2.0})};
  net.fixed = {{0}, vec({100.0})};
  report([&] { (void)th::steady(net); });             // node 2 has no link
  th::Net open = net;
  open.fixed = {};
  report([&] { (void)th::steady(open); });            // nothing fixes the temperature
  th::Net far = net;
  far.links.b = {7};
  report([&] { (void)th::steady(far); });             // a node index outside the network
  th::Net loop = net;
  loop.links.b = {0};
  report([&] { (void)th::steady(loop); });            // a link from a node to itself
  th::Net dead = net;
  dead.links.conductance = vec({0.0});
  report([&] { (void)th::steady(dead); });            // a conductance that is not positive
}
```

Output

```text
eng::thermal::steady: node 2 has no conduction path to a fixed temperature
eng::thermal::steady: needs at least one fixed temperature
eng::thermal::steady: a link node index 7 is outside 0..2
eng::thermal::steady: a link joins node 0 to itself
eng::thermal::steady: links.conductance must be positive
```
The other mistake produces no error: temperature scales that disagree. The boundary below is in degrees Celsius, the starting temperature in kelvin, and the network faithfully cools a body from 293 degrees to 20.

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
  // A network is blind to the temperature scale, so mixing scales gives a wrong answer without an error:
  // the boundary is at 20 degrees Celsius, but the body was started at 293.15 (kelvin).
  th::Net net;
  net.nodes = 2;
  net.links = {{0}, {1}, vec({0.5})};
  net.stores = {{0}, vec({100.0})};
  net.fixed = {{1}, vec({20.0})};
  th::Start mixed, consistent;
  mixed.temperature = vec({293.15});
  consistent.temperature = vec({20.0});
  const pluss::Series time = vec(grid(200.0, 4));
  out.writeln("mixed scales   ", flat(th::tran(net, time, {}, mixed).temperature.select(1, 0)), " the body appears to cool to 20 from 293");
  out.writeln("one scale      ", flat(th::tran(net, time, {}, consistent).temperature.select(1, 0)), " the body is already at the boundary");
}
```

Output

```text
mixed scales    [293.15000, 111.05000, 50.35000, 30.11667]  the body appears to cool to 20 from 293
one scale       [20.00000, 20.00000, 20.00000, 20.00000]  the body is already at the boundary
```
Pick one scale for the whole network and say so in the program.

## Where next

Reference pages: [`Net`](../../../../api/eng/thermal/network/Net.md), [`Links`](../../../../api/eng/thermal/network/Links.md), [`Fixed`](../../../../api/eng/thermal/network/Fixed.md), [`Heats`](../../../../api/eng/thermal/network/Heats.md), [`steady`](../../../../api/eng/thermal/network/steady.md), [`Result`](../../../../api/eng/thermal/network/Result.md), [`wall`](../../../../api/eng/thermal/conduction/wall.md). The next page adds time: [Transient response](transient_response.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="transient_response.html">
    <span class="label">Next</span>
    <span class="title">Transient response</span>
  </a>
</div>
<!-- apiref:nav:end -->
