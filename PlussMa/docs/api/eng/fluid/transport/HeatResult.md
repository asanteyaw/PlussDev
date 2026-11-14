<!-- generated from HeatResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `fluid::HeatResult`

The temperatures at every hydraulic event, with the energy balance.

## Signature

```cpp
fluid::HeatResult
```

## What It Does

The result of `heat`: the hydraulic `RunResult` that drove it, the temperatures sampled at the same event times and the stored energy.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `hydraulic` | The `RunResult` used. |  |
| `node` | Mixed temperature, K, `[sample, ..., node]`. |  |
| `link` | Volume mean temperature of a pipe, upstream temperature of a pump or valve, K, `[sample, ..., link]`. |  |
| `tank` | Temperature of each fully mixed tank, K, `[sample, ..., tank]`. |  |
| `energy` | Stored sensible energy `ρ c_p Σ V T` in the pipes and tanks, J, `[sample, ...]`. |  |
| `energy_error` | Storage change minus the ambient exchange and boundary flux, J, `[sample, ...]`. |  |

## Returns

Returned by `heat`.

## Errors

Reading a field does not run again.

## Interpretation and Use

The stored energy is counted from absolute zero so that boundary flows are unambiguous. `energy_error` should stay at rounding level; an exact exponential, not `UA (T - Ta) dt`, is used for the ambient exchange.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A pipe cooling, with the energy fields.

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

Shapes and the flow-weighted mixture.

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

// two reservoirs feed junction 2 through pipes of different length
static fluid::Net two_sources() {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 0.0, 5.0});
  net.nodes.demand = vec({0.0, 0.0, 0.02});
  net.nodes.head = vec({50.0, 48.0, 40.0});
  net.pipes.from = {0, 1};
  net.pipes.to = {2, 2};
  net.pipes.length = vec({600.0, 300.0});
  net.pipes.diameter = vec({0.15, 0.15});
  net.pipes.roughness = vec({1e-4, 1e-4});
  net.pipes.minor = vec({0.0, 0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto network = fluid::network(two_sources());
  const auto flow = flat(fluid::steady(network).flow);
  fluid::Schedule schedule;
  schedule.duration = 3600.0;
  schedule.step = 120.0;
  schedule.pattern_step = 24.0 * 3600.0;
  fluid::Heat heat;
  heat.initial = vec({340.0, 290.0, 300.0});
  fluid::RouteOptions route;
  route.step = 10.0;
  route.courant = 0.9;
  route.cells = 20;
  const auto r = fluid::heat(network, schedule, heat, route);
  const auto node = flat(r.node);
  out.writeln("shapes: node ", r.node.sizes(), ", link ", r.link.sizes(), ", tank ", r.tank.sizes(), ", energy ", r.energy.sizes());
  out.writeln("junction ", node.back(), " K, flow-weighted average ", (flow[0] * 340.0 + flow[1] * 290.0) / (flow[0] + flow[1]), " K");
  out.writeln("energy error ", flat(r.energy_error).back(), " J; the hydraulic trajectory has ", r.hydraulic.time.numel(), " samples");
}
```

Output

```text
shapes: node [31, 3], link [31, 2], tank [31, 0], energy [31]
junction 322.8263 K, flow-weighted average 322.8263 K
energy error 0.0004 J; the hydraulic trajectory has 31 samples
```
### Example 3

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

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Heat.html">
    <span class="label">Previous</span>
    <span class="title">Heat</span>
  </a>
  <a class="next-link" href="heat_fn.html">
    <span class="label">Next</span>
    <span class="title">heat</span>
  </a>
</div>
<!-- apiref:nav:end -->
