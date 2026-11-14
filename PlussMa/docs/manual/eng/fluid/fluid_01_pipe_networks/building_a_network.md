<!-- generated from building_a_network.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Building a network

[Back to Fluid 1: Pipe Networks](../fluid_01_pipe_networks.md)

A water system is a set of pipes joining reservoirs, tanks and the junctions where people draw water. You describe it once, as arrays of elevations, demands and pipe dimensions, and ask what heads and flows it settles into. `fluid::network` checks the description and compiles it, and `fluid::steady` solves it. This page explains the description, solves the smallest network there is and checks it against the textbook formula, shows how flow direction and the sign of a demand are read, shows what the fluid itself changes, and lists the descriptions the compiler refuses.

## The mental model

A network is **nodes** joined by **links**. A node is a `Junction` (its head is unknown and a demand leaves there), a `Reservoir` (its head is fixed, and it supplies or absorbs whatever flow is needed) or a `Tank` (a fixed-grade node whose level changes in time, covered in [Fluid 3](../fluid_03_storage_and_operation.md)). A link is a pipe, a pump or a valve; a pipe has a `from` node and a `to` node.

The unknowns are the **head** at every junction, `h = elevation + pressure head`, in metres, and the **flow** in every link, in cubic metres per second. Two laws determine them. Continuity at each junction says that what flows in minus what flows out equals the demand. Energy along each pipe says that the head drops by the head loss, which grows with the square of the flow. A reservoir fixes the absolute level of the heads, and so every connected part of the network needs one.

Two conventions fix every sign. A flow is positive when it runs from the pipe's `from` node to its `to` node, and a negative flow is a valid result that means the water runs the other way. A demand is positive when water leaves the network at that node, and a negative demand is a source.

The description is a `Net`, built from groups of arrays. The last axis of each array is its node or its pipe, so a leading axis is a batch of scenarios (see [Scenarios and sensitivities](scenarios_and_sensitivities.md)).

| Group | Member | Meaning |
|---|---|---|
| `nodes` | `kind` | one `NodeKind` per node |
| | `elevation`, `head` | node elevation in m; the fixed head at a reservoir and a starting value elsewhere |
| | `demand` | m³/s leaving the network; zero at a reservoir |
| | `emitter` | optional emitter coefficient (see [Demand policy](../fluid_03_storage_and_operation/demand_policy.md)) |
| `pipes` | `from`, `to` | node indices of the two ends |
| | `length`, `diameter` | in m |
| | `roughness` | absolute roughness in m for the Darcy law, the coefficient `C` for Hazen-Williams |
| | `minor` | optional dimensionless minor-loss coefficient `K` |

`pumps`, `valves` and `tanks` stay empty here and are described in the next chapters. `network(net, liquid)` returns an immutable compiled `Network`; `steady(network, options)` returns a `SteadyResult` with the head, pressure, flow, velocity, head loss and friction factor of every node and link, and the residuals that show how well the equations hold.

## One pipe, checked by hand

The smallest network is a reservoir at `50` m feeding one junction through a kilometre of pipe, `0.30` m in diameter and `1e-4` m rough, with `0.02` m³/s drawn at the junction. With only one pipe the flow is the demand, and the head loss follows from the Darcy-Weisbach law, `h = f (L/D) v^2 / (2 g)`, with the friction factor `f` from the Swamee-Jain relation for turbulent flow. The program solves the network and then computes the same numbers directly. The counts come from the `Network` accessors; `links` is pipes plus pumps plus valves.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a reservoir at 50 m feeding one junction through a 1 km pipe
static fluid::Net one_pipe(double demand) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 5.0});
  net.nodes.demand = vec({0.0, demand});
  net.nodes.head = vec({50.0, 49.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({0.30});
  net.pipes.roughness = vec({1.0e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const auto network = fluid::network(one_pipe(0.02));
  out.writeln("nodes ", network.nodes(), ", junctions ", network.junctions(), ", reservoirs ", network.reservoirs(), ", tanks ", network.tanks());
  out.writeln("pipes ", network.pipes(), ", pumps ", network.pumps(), ", valves ", network.valves(), ", links ", network.links());
  const auto r = fluid::steady(network);
  out.writeln("junction head ", flat(r.head)[1], " m, flow ", flat(r.flow)[0], " m3/s, velocity ", flat(r.velocity)[0], " m/s");
  out.writeln("pressure ", flat(r.pressure)[1], " Pa gauge, head loss ", flat(r.head_loss)[0], " m, friction ", flat(r.friction_factor)[0]);

  // by hand: Swamee-Jain friction factor and Darcy-Weisbach
  const double D = 0.30, L = 1000.0, eps = 1.0e-4, q = 0.02, nu = 1.0038e-6, g = 9.80665;
  const double pi = 3.14159265358979323846;
  const double v = 4.0 * q / (pi * D * D);
  const double Re = v * D / nu;
  const double lg = std::log10(eps / (3.7 * D) + 5.74 / std::pow(Re, 0.9));
  const double f = 0.25 / (lg * lg);
  const double hl = f * L / D * v * v / (2.0 * g);
  out.writeln("by hand: v ", v, ", Re ", Re, ", f ", f, ", head loss ", hl, ", junction head ", 50.0 - hl);
  out.writeln("pressure from head: ", 998.2 * g * (50.0 - hl - 5.0), " Pa");
}
```

Output

```text
nodes 2, junctions 1, reservoirs 1, tanks 0
pipes 1, pumps 0, valves 0, links 1
junction head 49.725662 m, flow 0.020000 m3/s, velocity 0.282942 m/s
pressure 437819.413252 Pa gauge, head loss 0.274338 m, friction 0.020163
by hand: v 0.282942, Re 84561.303363, f 0.020163, head loss 0.274338, junction head 49.725662
pressure from head: 437819.413252 Pa
```

The solver and the formula agree to every printed digit: the junction head is `49.725662` m, `0.274338` m below the reservoir. The velocity is `0.283` m/s, the Reynolds number `84561` puts the flow well into the turbulent range, and the pressure is the head above the junction's `5` m elevation, converted with the density and gravity of the liquid: `998.2 × 9.80665 × 44.725662 = 437819` Pa gauge. The pressure is a gauge pressure, so a value below zero means suction.

## Direction and the sign of a demand

Declaring the same pipe the other way round, from the junction to the reservoir, does not change the physics, only the sign of the result: the flow is then negative, because the water runs against the declared direction. A negative demand turns the junction into a source that pushes water into the pipe, so the head at the junction rises above the reservoir and the flow runs back to it. The program solves both variants of the network of the previous program.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static fluid::Net one_pipe(double demand) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 5.0});
  net.nodes.demand = vec({0.0, demand});
  net.nodes.head = vec({50.0, 49.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({0.30});
  net.pipes.roughness = vec({1.0e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const auto forward = fluid::steady(fluid::network(one_pipe(0.02)));
  out.writeln("reservoir -> junction, demand +0.02: flow ", flat(forward.flow)[0], ", junction head ", flat(forward.head)[1]);

  // the same pipe, declared junction -> reservoir
  auto reversed = one_pipe(0.02);
  reversed.pipes.from = {1};
  reversed.pipes.to = {0};
  const auto backward = fluid::steady(fluid::network(reversed));
  out.writeln("junction -> reservoir, demand +0.02: flow ", flat(backward.flow)[0], ", junction head ", flat(backward.head)[1]);

  // a source at the junction instead of a demand
  const auto source = fluid::steady(fluid::network(one_pipe(-0.02)));
  out.writeln("reservoir -> junction, demand -0.02: flow ", flat(source.flow)[0], ", junction head ", flat(source.head)[1]);
}
```

Output

```text
reservoir -> junction, demand +0.02: flow 0.020000, junction head 49.725662
junction -> reservoir, demand +0.02: flow -0.020000, junction head 49.725662
reservoir -> junction, demand -0.02: flow -0.020000, junction head 50.274338
```

Reversing the declaration flips the sign of the flow, `-0.02`, and leaves the head at `49.725662` m, the same as before. A source of `0.02` m³/s at the junction gives the same flow, `-0.02`, because the water now runs up to the reservoir, and a junction head of `50.274338` m, as far above the reservoir as the demand had put it below. You never need to order the endpoints to match the expected flow: read the sign.

## What the liquid changes

`network(net, liquid)` takes a `Liquid`: a density in kg/m³, a kinematic viscosity in m²/s and the gravity in m/s², by default water at 20 °C, `998.2`, `1.0038e-6` and `9.80665`. The density and gravity turn heads into pressures. The viscosity sets the Reynolds number and, through it, the friction factor. The next program solves the same pipe with water at 20 °C, water at 80 °C and an oil about a hundred times as viscous as water.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static fluid::Net one_pipe(double demand, double diameter) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 5.0});
  net.nodes.demand = vec({0.0, demand});
  net.nodes.head = vec({50.0, 49.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({diameter});
  net.pipes.roughness = vec({1.0e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  // water at 20 C, water at 80 C, and an oil about 100 times more viscous
  const double nu[3] = {1.0038e-6, 0.3646e-6, 1.0e-4};
  const double rho[3] = {998.2, 971.8, 870.0};
  const char *name[3] = {"water 20 C", "water 80 C", "oil       "};
  for (int i = 0; i < 3; ++i) {
    fluid::Liquid liquid;
    liquid.density = rho[i];
    liquid.kinematic_viscosity = nu[i];
    const auto r = fluid::steady(fluid::network(one_pipe(0.02, 0.30), liquid));
    out.writeln(name[i], ": Reynolds ", 4.0 * 0.02 / (3.14159265358979 * 0.30 * nu[i]), ", friction ", flat(r.friction_factor)[0], ", head loss ", flat(r.head_loss)[0], " m, pressure at the junction ", flat(r.pressure)[1] / 1000.0, " kPa");
  }
}
```

Output

```text
water 20 C: Reynolds 84561.303363, friction 0.020163, head loss 0.274338 m, pressure at the junction 437.819413 kPa
water 80 C: Reynolds 232810.302566, friction 0.017710, head loss 0.240963 m, pressure at the junction 426.558208 kPa
oil       : Reynolds 848.826363, friction 0.075398, head loss 1.025851 m, pressure at the junction 375.178005 kPa
```

Hot water has a lower viscosity, so its Reynolds number is higher, its friction factor drops from `0.020163` to `0.017710` and the head loss from `0.274` m to `0.241` m. Its density is also lower, so the same head is a lower pressure, `426.6` kPa against `437.8` kPa. The oil is laminar at a Reynolds number of `849`, its friction factor is `64 / Re = 0.0754`, and the head loss is almost four times that of water. The heads and flows of the network depend on the liquid only through the viscosity; the pressures depend on the density as well.

## Decisions

- **Units.** Everything is SI: metres, cubic metres per second, kilograms per cubic metre, seconds. There is no unit conversion in the interface.
- **Reservoir or tank.** Use a `Reservoir` for a source whose level does not change during the study and a `Tank` for storage whose level does. A single steady solve treats a tank as a fixed head at its initial level.
- **Pipe orientation.** Declare pipes in the direction you expect the flow to run, so the results read as positive numbers, and trust the sign when a flow reverses.
- **Precision.** `Float64` is the default. A network built from `Float32` arrays is solved and returned in `Float32`, which suits large scenario batches; set the tolerances of the solver to match, as described in [Looped networks](looped_networks.md).

## Pitfalls

Three mistakes, each in a program that makes it on purpose. The first leaves a pair of pipes unconnected to any reservoir, and the second writes a demand at the reservoir. The third asks one pipe to deliver more than it can at a positive pressure; it is accepted, and the pressure that comes back is the warning.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <iostream>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static fluid::Net one_pipe(double demand) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 5.0});
  net.nodes.demand = vec({0.0, demand});
  net.nodes.head = vec({50.0, 49.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({0.30});
  net.pipes.roughness = vec({1.0e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));

  // 1. a second pipe that is not connected to the reservoir
  try {
    auto net = one_pipe(0.01);
    net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Junction, fluid::NodeKind::Junction};
    net.nodes.elevation = vec({0.0, 5.0, 5.0, 5.0});
    net.nodes.demand = vec({0.0, 0.01, 0.01, 0.01});
    net.nodes.head = vec({50.0, 49.0, 49.0, 49.0});
    net.pipes.from = {0, 2};
    net.pipes.to = {1, 3};
    net.pipes.length = vec({1000.0, 1000.0});
    net.pipes.diameter = vec({0.30, 0.30});
    net.pipes.roughness = vec({1.0e-4, 1.0e-4});
    net.pipes.minor = vec({0.0, 0.0});
    (void)fluid::network(net);
  } catch (const std::exception &e) {
    std::cout << "a pipe pair with no reservoir: " << e.what() << "\n";
  }

  // 2. a demand written at the reservoir
  try {
    auto net = one_pipe(0.02);
    net.nodes.demand = vec({0.02, 0.0});
    (void)fluid::network(net);
  } catch (const std::exception &e) {
    std::cout << "a demand at the reservoir: " << e.what() << "\n";
  }

  // 3. a demand that the pipe cannot deliver at a positive pressure
  for (double demand : {0.2, 0.6, 1.0}) {
    const auto r = fluid::steady(fluid::network(one_pipe(demand)));
    out.writeln("demand ", demand, " m3/s: junction pressure ", flat(r.pressure)[1] / 1000.0, " kPa, delivered ", flat(r.demand)[1]);
  }
}
```

Output

```text
a pipe pair with no reservoir: eng::fluid::network: every connected component needs a reservoir or tank
a demand at the reservoir: eng::fluid::network: reservoir and tank demand/emitter lanes must be zero
demand 0.200 m3/s: junction pressure 225.257 kPa, delivered 0.200
demand 0.600 m3/s: junction pressure -1432.341 kPa, delivered 0.600
demand 1.000 m3/s: junction pressure -4720.158 kPa, delivered 1.000
```

A part of the network with no reservoir has no absolute head, so the compiler refuses it; join it to a reservoir or give it one. A reservoir supplies whatever flow is needed, so a demand written there has no meaning and is refused. The third case is the dangerous one. A fixed demand is a promise that the water is delivered whatever the pressure, so the solver delivers `0.600` m³/s and reports a pressure of `-1432` kPa, below a vacuum, and `1.000` m³/s at `-4720` kPa. The mistake is to read the delivered demand without looking at the pressure. When pressures can fall that low, check them, or let the demand fall with the pressure by solving with the pressure-driven policy of [Demand policy](../fluid_03_storage_and_operation/demand_policy.md).

Reference: [`network`](../../../../api/eng/fluid/model/network_fn.md), [`steady`](../../../../api/eng/fluid/steady/steady.md), [`Net`](../../../../api/eng/fluid/model/Net.md), [`Liquid`](../../../../api/eng/fluid/model/Liquid.md), [`SteadyResult`](../../../../api/eng/fluid/steady/SteadyResult.md), [`Network`](../../../../api/eng/fluid/model/Network.md).

Where next: [Looped networks](looped_networks.md) solves a network where the water has more than one way to go.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="looped_networks.html">
    <span class="label">Next</span>
    <span class="title">Looped networks</span>
  </a>
</div>
<!-- apiref:nav:end -->
