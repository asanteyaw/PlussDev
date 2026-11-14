<!-- generated from control_valves.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Control valves

[Back to Fluid 2: Pumps and Valves](../fluid_02_pumps_and_valves.md)

A pump adds head to the water; a valve takes it away on purpose. Valves cap the pressure in a low-lying district, hold the flow into a treatment plant to a set rate, keep a high zone from being drained, and throttle one branch so that another receives its share. `Net::valves` describes four kinds of valve, and each is a different equation. This page explains what each kind demands of the network, solves each against a result you can check, and shows the settings the network cannot satisfy, which the solver refuses.

## The mental model

A valve is a link from an upstream node to a downstream node, numbered after the pipes and the pumps. A valve never adds head: the head across it, `h_from - h_to`, is zero or positive, and it is reported in `SteadyResult::head_loss` like any other link. What distinguishes the four `ValveKind`s is which equation replaces the ordinary head-loss law of the link.

| Kind | Equation the solver enforces | `setting` is | Use it to |
|---|---|---|---|
| `Throttle` | `h_from - h_to = K v ∣v∣ / (2 g)` | unused (the loss coefficient is `minor`) | add a known loss, a partly closed valve |
| `Flow` | flow through the valve `= setting` | a flow in m³/s | hold a fixed rate |
| `Reduce` | `h_to = elevation_to + setting` | a pressure head in m | cap the pressure downstream |
| `Sustain` | `h_from = elevation_from + setting` | a pressure head in m | hold the pressure upstream |

A valve is described by `kind`, `from`, `to`, its `diameter`, its loss coefficient `minor` and its `setting`, one entry per valve. The diameter turns the flow into a velocity for the throttle loss. The two pressure valves, `Reduce` and `Sustain`, regulate the pressure at a junction.

The three controls are equations that a network can fail to satisfy. A pressure-reducing valve has to be the only way water reaches its downstream junction, or the other feed would swamp it. A sustaining valve can only hold an upstream pressure that is above the pressure the junction would have without it, because a valve can only throttle. A fixed-flow valve can only hold a flow that the head difference across it can pass. A setting that breaks one of these is refused with an error that says which, as the last section shows.

## A throttle valve

Reservoir `0` at `80` m feeds junction `1` through a pipe and junction `2` through another, and a throttle valve joins junction `1` to junction `2`. Each junction draws `0.02` m³/s. The pipes are `0.20` m and `0.15` m in diameter, `600` m and `900` m long. The program solves the network with the valve's loss coefficient at `0`, `10`, `100` and `1000`, and checks the head across the valve against `K v² / (2 g)` for the velocity in its `0.20` m bore.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <iostream>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// reservoir 0 (80 m) feeds junction 1 through one pipe and junction 2 through another; a valve joins 1 -> 2
static fluid::Net with_valve(fluid::ValveKind kind, double setting, double K) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 10.0, 5.0});
  net.nodes.demand = vec({0.0, 0.02, 0.02});
  net.nodes.head = vec({80.0, 55.0, 55.0});
  net.pipes.from = {0, 0};
  net.pipes.to = {1, 2};
  net.pipes.length = vec({600.0, 900.0});
  net.pipes.diameter = vec({0.20, 0.15});
  net.pipes.roughness = vec({1e-4, 1e-4});
  net.pipes.minor = vec({0.0, 0.0});
  net.valves.kind = {kind};
  net.valves.from = {1};
  net.valves.to = {2};
  net.valves.diameter = vec({0.20});
  net.valves.minor = vec({K});
  net.valves.setting = vec({setting});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const double kPi = 3.14159265358979323846, g = 9.80665;
  for (double K : {0.0, 10.0, 100.0, 1000.0}) {
    const auto r = fluid::steady(fluid::network(with_valve(fluid::ValveKind::Throttle, 0.0, K)));
    const auto q = flat(r.flow), h = flat(r.head);
    const double v = 4.0 * q[2] / (kPi * 0.2 * 0.2);
    out.writeln("K = ", K, ": valve flow ", q[2], ", head across the valve ", h[1] - h[2], ", K v^2/2g = ", K * v * v / (2.0 * g), ", junction heads ", h[1], " ", h[2]);
  }
}
```

Output

```text
K = 0.00000: valve flow 0.00906, head across the valve 0.00000, K v^2/2g = 0.00000, junction heads 77.50322 77.50322
K = 10.00000: valve flow 0.00899, head across the valve 0.04175, K v^2/2g = 0.04175, junction heads 77.51471 77.47296
K = 100.00000: valve flow 0.00845, head across the valve 0.36874, K v^2/2g = 0.36874, junction heads 77.60244 77.23370
K = 1000.00000: valve flow 0.00608, head across the valve 1.90664, K v^2/2g = 1.90664, junction heads 77.96859 76.06195
```

With `K = 0` the valve is fully open and passes `0.00906` m³/s, the share the two junctions' heads allow. Closing it down shrinks that flow, to `0.00608` at `K = 1000`, and the head across it matches `K v² / (2 g)` to every printed digit: `1.90664` m at `K = 1000`. The water it no longer carries is taken by the pipe to junction `2`, which has to run harder, so junction `2` falls from `77.50` m to `76.06` m while junction `1` rises. A throttle valve moves the flow without choosing it.

## A flow valve and a pressure-reducing valve

A `Flow` valve fixes the flow through itself. In the same network the valve is set to `0.005` and to `0.009` m³/s: the rest of the water has to go by the pipes. A `Reduce` valve fixes the pressure downstream, and it needs a network in which it is the only way in, so junction `2` loses its own pipe: the reservoir feeds junction `1` through the larger pipe, and the valve alone feeds junction `2`. The program sets the downstream pressure head to `60`, `40` and `20` m.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// reservoir 0 (80 m) feeds junction 1 through a pipe; junction 2 is fed through a second pipe or only through the valve
static fluid::Net with_valve(fluid::ValveKind kind, double setting, bool second_pipe) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 10.0, 5.0});
  net.nodes.demand = vec({0.0, 0.02, 0.02});
  net.nodes.head = vec({80.0, 55.0, 55.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({600.0});
  net.pipes.diameter = vec({0.20});
  net.pipes.roughness = vec({1e-4});
  net.pipes.minor = vec({0.0});
  if (second_pipe) {
    net.pipes.from = {0, 0};
    net.pipes.to = {1, 2};
    net.pipes.length = vec({600.0, 900.0});
    net.pipes.diameter = vec({0.20, 0.15});
    net.pipes.roughness = vec({1e-4, 1e-4});
    net.pipes.minor = vec({0.0, 0.0});
  }
  net.valves.kind = {kind};
  net.valves.from = {1};
  net.valves.to = {2};
  net.valves.diameter = vec({0.20});
  net.valves.minor = vec({0.0});
  net.valves.setting = vec({setting});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  for (double s : {0.005, 0.009}) {
    const auto r = fluid::steady(fluid::network(with_valve(fluid::ValveKind::Flow, s, true)));
    const auto q = flat(r.flow), h = flat(r.head);
    out.writeln("flow valve set to ", s, ": flows (pipe, pipe, valve) ", q, ", junction heads ", h[1], " ", h[2], ", head across the valve ", flat(r.head_loss)[2]);
  }
  for (double s : {60.0, 40.0, 20.0}) {
    const auto r = fluid::steady(fluid::network(with_valve(fluid::ValveKind::Reduce, s, false)));
    const auto q = flat(r.flow), h = flat(r.head);
    out.writeln("reducing valve set to ", s, " m: pressure head at junction 2 ", h[2] - 5.0, ", junction 1 head ", h[1], ", flows (pipe, valve) ", q, ", head across the valve ", flat(r.head_loss)[1]);
  }
}
```

Output

```text
flow valve set to 0.00500: flows (pipe, pipe, valve) [0.02500, 0.01500, 0.00500], junction heads 78.12469 75.46508, head across the valve 2.65961
flow valve set to 0.00900: flows (pipe, pipe, valve) [0.02900, 0.01100, 0.00900], junction heads 77.51313 77.47713, head across the valve 0.03600
reducing valve set to 60.00000 m: pressure head at junction 2 60.00000, junction 1 head 75.39810, flows (pipe, valve) [0.04000, 0.02000], head across the valve 10.39810
reducing valve set to 40.00000 m: pressure head at junction 2 40.00000, junction 1 head 75.39810, flows (pipe, valve) [0.04000, 0.02000], head across the valve 30.39810
reducing valve set to 20.00000 m: pressure head at junction 2 20.00000, junction 1 head 75.39810, flows (pipe, valve) [0.04000, 0.02000], head across the valve 50.39810
```

The flow valve does what it is told: the valve carries exactly `0.00500` and `0.00900`, and the two pipes make up the `0.04` total in the shares the junction heads allow. The reducing valve holds the pressure head downstream at its setting, `60.0`, `40.0` and `20.0` m, whatever the setting, and the flow through it is always the `0.02` m³/s that junction `2` draws. What changes is the head the valve burns: `10.40` m at a setting of `60` m, and `50.40` m at `20` m. The junction upstream stays at `75.40` m in all three, because the flow it carries does not change.

## A sustaining valve

A `Sustain` valve holds the pressure upstream of itself, by throttling whatever is downstream. It acts where the pressure upstream would otherwise sag, so its setting has to lie above the pressure that junction `1` has without it, which is `67.50` m of pressure head here, and below what the reservoir can lift it to, `70` m. The program sets it to `68` m and shows the water it has had to hold back.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static fluid::Net with_valve(fluid::ValveKind kind, double setting) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 10.0, 5.0});
  net.nodes.demand = vec({0.0, 0.02, 0.02});
  net.nodes.head = vec({80.0, 55.0, 55.0});
  net.pipes.from = {0, 0};
  net.pipes.to = {1, 2};
  net.pipes.length = vec({600.0, 900.0});
  net.pipes.diameter = vec({0.20, 0.15});
  net.pipes.roughness = vec({1e-4, 1e-4});
  net.pipes.minor = vec({0.0, 0.0});
  net.valves.kind = {kind};
  net.valves.from = {1};
  net.valves.to = {2};
  net.valves.diameter = vec({0.20});
  net.valves.minor = vec({0.0});
  net.valves.setting = vec({setting});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const auto natural = fluid::steady(fluid::network(with_valve(fluid::ValveKind::Throttle, 0.0)));
  out.writeln("without a regulating valve: junction 1 pressure head ", flat(natural.head)[1] - 10.0, ", valve flow ", flat(natural.flow)[2]);
  const auto r = fluid::steady(fluid::network(with_valve(fluid::ValveKind::Sustain, 68.0)));
  const auto q = flat(r.flow), h = flat(r.head);
  out.writeln("sustaining valve set to 68 m: junction 1 pressure head ", h[1] - 10.0, ", flows (pipe, pipe, valve) ", q, ", junction 2 head ", h[2], ", head across the valve ", flat(r.head_loss)[2]);
}
```

Output

```text
without a regulating valve: junction 1 pressure head 67.50322, valve flow 0.00906
sustaining valve set to 68 m: junction 1 pressure head 68.00000, flows (pipe, pipe, valve) [0.02586, 0.01414, 0.00586], junction 2 head 75.94696, head across the valve 2.05304
```

Without the valve junction `1` sits at a pressure head of `67.50` m and `0.00906` m³/s crosses to junction `2`. The sustaining valve raises the junction to `68.00` m by cutting the cross flow to `0.00586` m³/s: less water leaves junction `1` and its supply pipe carries `0.02586` instead of `0.02906`. Junction `2` makes up the shortfall through its own pipe, and the valve burns `2.05` m of head to do it.

## Decisions

- **What to control.** A throttle valve adds a known resistance, a flow valve fixes a rate, a reducing valve caps a pressure below it and a sustaining valve holds a pressure above it. Pick the one whose quantity you want to hold, and the equation does the rest.
- **Where the valve sits.** A reducing valve belongs on the only path into the zone it protects; a sustaining valve on a junction that has another way to be supplied, so the water it holds back has somewhere to go.
- **Setting versus the loss coefficient.** `minor` is a loss coefficient and applies to a `Throttle` valve. A `Flow`, `Reduce` or `Sustain` valve takes its `setting` and ignores `minor` while it regulates.
- **Opening and closing a valve over time.** A valve here is always active. Switching one on a schedule or on a tank level is the subject of [Controls and rules](../fluid_03_storage_and_operation/controls_and_rules.md).

## Pitfalls

Each of these is a setting the network cannot satisfy. The first asks the flow valve for `0.02` m³/s when the head difference across it passes only `0.009`. The second sets the sustaining valve below the `67.5` m the junction has anyway. The third puts a reducing valve in parallel with a pipe that feeds the same junction. In each case the solver would need a valve that adds head, or flows backwards, and it refuses.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <iostream>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

static fluid::Net with_valve(fluid::ValveKind kind, double setting) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 10.0, 5.0});
  net.nodes.demand = vec({0.0, 0.02, 0.02});
  net.nodes.head = vec({80.0, 55.0, 55.0});
  net.pipes.from = {0, 0};
  net.pipes.to = {1, 2};
  net.pipes.length = vec({600.0, 900.0});
  net.pipes.diameter = vec({0.20, 0.15});
  net.pipes.roughness = vec({1e-4, 1e-4});
  net.pipes.minor = vec({0.0, 0.0});
  net.valves.kind = {kind};
  net.valves.from = {1};
  net.valves.to = {2};
  net.valves.diameter = vec({0.20});
  net.valves.minor = vec({0.0});
  net.valves.setting = vec({setting});
  return net;
}

static void attempt(const char *label, fluid::ValveKind kind, double setting) {
  try {
    (void)fluid::steady(fluid::network(with_valve(kind, setting)));
    std::cout << label << ": solved\n";
  } catch (const std::exception &e) {
    std::cout << label << ": " << e.what() << "\n";
  }
}

int main() {
  attempt("flow valve set to 0.02 m3/s (it can pass 0.009)", fluid::ValveKind::Flow, 0.02);
  attempt("sustaining valve set to 60 m (the junction has 67.5 m)", fluid::ValveKind::Sustain, 60.0);
  attempt("sustaining valve set to 69 m (the reservoir cannot lift it that far)", fluid::ValveKind::Sustain, 69.0);
  attempt("reducing valve set to 40 m with a second pipe into junction 2", fluid::ValveKind::Reduce, 40.0);
}
```

Output

```text
flow valve set to 0.02 m3/s (it can pass 0.009): eng::fluid::steady: an active control valve cannot add head
sustaining valve set to 60 m (the junction has 67.5 m): eng::fluid::steady: an active control valve cannot add head
sustaining valve set to 69 m (the reservoir cannot lift it that far): eng::fluid::steady: an active control valve requires reverse flow
reducing valve set to 40 m with a second pipe into junction 2: eng::fluid::steady: an active control valve requires reverse flow
```

The first two ask the valve to add head, `an active control valve cannot add head`: a valve can only take head out, so a flow above what the head difference passes, or an upstream pressure below the natural one, is out of its reach. The third sets the pressure beyond what the reservoir can deliver, so the valve would have to run backwards: `requires reverse flow`. The fourth fails the same way, because the second pipe already fills junction `2` and leaves the valve nothing to carry. Each message names the setting to change. If the network is right and the setting is wrong, move the setting inside what the network can do; if you wanted the setting, change the network, as the section on where the valve sits describes.

Reference: [`Valves`](../../../../api/eng/fluid/model/Valves.md), [`ValveKind`](../../../../api/eng/fluid/model/ValveKind.md), [`steady`](../../../../api/eng/fluid/steady/steady.md), [`SteadyResult`](../../../../api/eng/fluid/steady/SteadyResult.md).

Where next: [Fluid 3: Storage and Operation](../fluid_03_storage_and_operation.md) lets the demands, the tanks and the settings change with time.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pumps.html">
    <span class="label">Previous</span>
    <span class="title">Pumps</span>
  </a>
</div>
<!-- apiref:nav:end -->
