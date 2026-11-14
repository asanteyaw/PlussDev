<!-- generated from Heat.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `fluid::Heat`

The temperatures, conductances and specific heat of a thermal run.

## Signature

```cpp
fluid::Heat
```

## What It Does

Describes a thermal run: absolute temperatures, the conductance of each pipe and tank to its ambient, and a constant specific heat. The model is one-way coupled: the temperature does not change the hydraulics.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `initial` | The starting temperature at every node, K, `[..., node]`. | required |
| `source` | The temperature held at a reservoir and of an injection, K, `[..., node]`. | `initial` |
| `ambient` | The surrounding temperature of each pipe, K, `[..., pipe]`. | required where `ua > 0` |
| `ua` | The total conductance of each pipe to its ambient, W/K, `[..., pipe]`. | undefined (zero) |
| `tank_ambient` | The surrounding temperature of each tank, K, `[..., tank]`. | required where `tank_ua > 0` |
| `tank_ua` | The total conductance of each tank, W/K, `[..., tank]`. | undefined (zero) |
| `cp` | The specific heat, J/(kg K), constant for the call. | 4180 |

## Returns

A value; validated by `heat`.

## Errors

A missing initial temperature, negative or non-finite temperatures, a non-positive `cp`, a negative conductance, a conductance without its ambient temperature, the wrong width, and mixed precision are rejected.

## Interpretation and Use

`ua` is a total in W/K for the whole pipe, the per-metre figure times the length, and it is the whole resistance from the bulk water to the ambient. Density comes from the `Liquid` of the network. Pumps add no heat.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A pipe cooling against its exact value.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <functional>
#include <iostream>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static void attempt(const char *label, const std::function<void()> &call) {
  try {
    call();
    std::cout << label << ": accepted\n";
  } catch (const std::exception &e) {
    std::cout << label << ": " << e.what() << "\n";
  }
}

// a reservoir at 50 m feeding a junction through a 1 km pipe of 0.15 m; the pipe holds 17.67 m3 and the draw is 0.01 m3/s
static fluid::Net line(double demand = 0.01) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 5.0});
  net.nodes.demand = vec({0.0, demand});
  net.nodes.head = vec({50.0, 49.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({0.15});
  net.pipes.roughness = vec({1e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

static const double kPi = 3.14159265358979323846;
static const double kVolume = kPi * 0.15 * 0.15 / 4.0 * 1000.0; // m3
static const double kTransit = kVolume / 0.01;                   // s

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto network = fluid::network(line());
  const double density = 998.2, cp = 4180.0;
  const std::int64_t cells = 20;
  fluid::Schedule schedule;
  schedule.duration = 4.0 * 3600.0;
  schedule.step = kTransit / cells;
  schedule.pattern_step = 24.0 * 3600.0;
  fluid::RouteOptions route;
  route.step = kTransit / cells;
  route.courant = 1.0;
  route.cells = cells;
  for (double ua : {0.0, 5000.0, 20000.0, 80000.0}) {
    fluid::Heat heat;
    heat.initial = vec({340.0, 288.0});
    heat.ambient = vec({288.0});
    heat.ua = vec({ua});
    const auto r = fluid::heat(network, schedule, heat, route);
    const double exact = 288.0 + 52.0 * std::exp(-ua * kTransit / (density * cp * kVolume));
    out.writeln("UA ", ua, " W/K: junction ", flat(r.node).back(), " K, exact ", exact, " K, energy ", flat(r.energy).back(), " J, energy error ", flat(r.energy_error).back(), " J");
  }
}
```

Output

```text
UA 0.0000 W/K: junction 340.0000 K, exact 340.0000 K, energy 25069470652.2325 J, energy error 0.0000 J
UA 5000.0000 W/K: junction 334.1276 K, exact 334.1276 K, energy 24837837101.9181 J, energy error 0.0000 J
UA 20000.0000 W/K: junction 320.1985 K, exact 320.1982 K, energy 24245008735.7612 J, energy error -0.0004 J
UA 80000.0000 W/K: junction 295.6448 K, exact 295.6440 K, energy 22860817189.1971 J, energy error 0.0001 J
```
### Example 2

A tank cooling.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <functional>
#include <iostream>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static void attempt(const char *label, const std::function<void()> &call) {
  try {
    call();
    std::cout << label << ": accepted\n";
  } catch (const std::exception &e) {
    std::cout << label << ": " << e.what() << "\n";
  }
}

// reservoir (60 m) -> pipe -> junction 1 (demand 0.04) -> pipe -> tank; the tank bottom is at 20 m, its surface starts at 23 m
static fluid::Net with_tank(double area = 400.0, bool overflow = false, double initial_head = 23.0) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Tank};
  net.nodes.elevation = vec({0.0, 10.0, 20.0});
  net.nodes.demand = vec({0.0, 0.04, 0.0});
  net.nodes.head = vec({60.0, 50.0, initial_head});
  net.tanks.area = vec({area});
  net.tanks.min_level = vec({0.5});
  net.tanks.max_level = vec({6.0});
  net.tanks.overflow = {overflow};
  net.pipes.from = {0, 1};
  net.pipes.to = {1, 2};
  net.pipes.length = vec({800.0, 400.0});
  net.pipes.diameter = vec({0.15, 0.20});
  net.pipes.roughness = vec({1e-4, 1e-4});
  net.pipes.minor = vec({0.0, 0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a tank holding 40 m2 x 3 m of water at 340 K cools towards 288 K; the reservoir is at the tank's head, so nothing flows
  auto net = with_tank(40.0, false);
  net.nodes.head = vec({23.0, 23.0, 23.0});
  net.nodes.demand = vec({0.0, 0.0, 0.0});
  const auto network = fluid::network(net);
  const double density = 998.2, cp = 4180.0, area = 40.0, level = 3.0, volume = area * level;
  fluid::Schedule schedule;
  schedule.duration = 3600.0;
  schedule.step = 600.0;
  schedule.pattern_step = 24.0 * 3600.0;
  fluid::RouteOptions route;
  route.step = 30.0;
  route.courant = 0.9;
  route.cells = 4;
  for (double ua : {0.0, 2000.0}) {
    fluid::Heat heat;
    heat.initial = vec({340.0, 340.0, 340.0});
    heat.ambient = vec({288.0, 288.0});
    heat.ua = vec({0.0, 0.0});
    heat.tank_ambient = vec({288.0});
    heat.tank_ua = vec({ua});
    const auto r = fluid::heat(network, schedule, heat, route);
    const double exact = 288.0 + 52.0 * std::exp(-ua * 3600.0 / (density * cp * volume));
    out.writeln("tank UA ", ua, " W/K: tank temperature after an hour ", flat(r.tank).back(), " K, exact ", exact, " K");
  }
}
```

Output

```text
tank UA 0.0000 W/K: tank temperature after an hour 340.0000 K, exact 340.0000 K
tank UA 2000.0000 W/K: tank temperature after an hour 339.2576 K, exact 339.2576 K
```
### Example 3

Descriptions that are refused.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <functional>
#include <iostream>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static void attempt(const char *label, const std::function<void()> &call) {
  try {
    call();
    std::cout << label << ": accepted\n";
  } catch (const std::exception &e) {
    std::cout << label << ": " << e.what() << "\n";
  }
}

// a reservoir at 50 m feeding a junction through a 1 km pipe of 0.15 m; the pipe holds 17.67 m3 and the draw is 0.01 m3/s
static fluid::Net line(double demand = 0.01) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 5.0});
  net.nodes.demand = vec({0.0, demand});
  net.nodes.head = vec({50.0, 49.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({0.15});
  net.pipes.roughness = vec({1e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

static const double kPi = 3.14159265358979323846;
static const double kVolume = kPi * 0.15 * 0.15 / 4.0 * 1000.0; // m3
static const double kTransit = kVolume / 0.01;                   // s

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const auto network = fluid::network(line());
  fluid::Schedule schedule;
  schedule.duration = 3600.0;
  schedule.step = 600.0;
  schedule.pattern_step = 24.0 * 3600.0;
  attempt("a conductance without an ambient temperature", [&] { fluid::Heat h; h.initial = vec({340.0, 288.0}); h.ua = vec({20000.0}); (void)fluid::heat(network, schedule, h); });
  attempt("a negative absolute temperature", [&] { fluid::Heat h; h.initial = vec({-10.0, 288.0}); (void)fluid::heat(network, schedule, h); });
  attempt("no initial temperature", [&] { fluid::Heat h; (void)fluid::heat(network, schedule, h); });
  attempt("a zero specific heat", [&] { fluid::Heat h; h.initial = vec({340.0, 288.0}); h.cp = 0.0; (void)fluid::heat(network, schedule, h); });
  attempt("a negative conductance", [&] { fluid::Heat h; h.initial = vec({340.0, 288.0}); h.ambient = vec({288.0}); h.ua = vec({-1.0}); (void)fluid::heat(network, schedule, h); });
}
```

Output

```text
a conductance without an ambient temperature: eng::fluid::heat: pipe ambient temperature is required
a negative absolute temperature: eng::fluid::heat: initial temperature must be nonnegative
no initial temperature: eng::fluid::heat: initial temperature is required
a zero specific heat: eng::fluid::heat: cp must be finite and positive
a negative conductance: eng::fluid::heat: ua values must be nonnegative
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="transport.html">
    <span class="label">Previous</span>
    <span class="title">transport</span>
  </a>
  <a class="next-link" href="HeatResult.html">
    <span class="label">Next</span>
    <span class="title">HeatResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
