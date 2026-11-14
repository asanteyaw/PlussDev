<!-- generated from NodeKind.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `fluid::NodeKind`

The three kinds of node of a pipe network.

## Signature

```cpp
fluid::NodeKind
```

## What It Does

Says what a node is. A `Junction` has an unknown head and a demand that leaves the network there. A `Reservoir` has a fixed head and supplies or absorbs whatever flow the network needs. A `Tank` is a fixed-grade node whose level changes in time under `run`; in a single `steady` solve it is held at its initial head.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `Junction` | A variable-head node. Its demand is positive when water leaves the network. |  |
| `Reservoir` | A fixed-head node of unlimited capacity. Its demand and emitter must be zero. |  |
| `Tank` | A cylindrical store with a level. It needs an entry in `Net::tanks`. |  |

## Returns

An enumeration; one value per node in `Nodes::kind`.

## Errors

A network with no `Reservoir` or `Tank` in a connected component, a `Reservoir` or `Tank` with a non-zero demand or emitter, and a tank without a `Tanks` entry are rejected by `network`.

## Interpretation and Use

Every connected part of the network needs at least one `Reservoir` or `Tank`, because only a fixed head anchors the absolute level of the heads. Use a `Tank` for storage whose level matters and a `Reservoir` for a source that does not change.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Node counts of a network and of a network with a tank.

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

static fluid::Net one_pipe_with_tank_entry() {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 5.0});
  net.nodes.demand = vec({0.0, 0.02});
  net.nodes.head = vec({50.0, 49.0});
  net.tanks.area = vec({400.0});
  net.tanks.min_level = vec({0.5});
  net.tanks.max_level = vec({6.0});
  net.tanks.overflow = {false};
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({0.30});
  net.pipes.roughness = vec({1e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto two = fluid::network(two_sources());
  out.writeln("two sources: ", two.reservoirs(), " reservoirs, ", two.junctions(), " junction, ", two.tanks(), " tanks");
  const auto tank = fluid::network(with_tank());
  out.writeln("with a tank: ", tank.reservoirs(), " reservoir, ", tank.junctions(), " junction, ", tank.tanks(), " tank");
  attempt("two junctions and no fixed-grade node", [] {
    auto net = two_sources();
    net.nodes.kind = {fluid::NodeKind::Junction, fluid::NodeKind::Junction, fluid::NodeKind::Junction};
    (void)fluid::network(net);
  });
  attempt("a tank node with no tank described", [] {
    auto net = with_tank();
    net.tanks = fluid::Tanks{};
    (void)fluid::network(net);
  });
  attempt("a tank described for a network that has none", [] {
    auto net = one_pipe_with_tank_entry();
    (void)fluid::network(net);
  });
}
```

Output

```text
two sources: 2 reservoirs, 1 junction, 0 tanks
with a tank: 1 reservoir, 1 junction, 1 tank
two junctions and no fixed-grade node: eng::fluid::network: every connected component needs a reservoir or tank
a tank node with no tank described: eng::fluid::network: tank overflow metadata must match tank nodes
a tank described for a network that has none: eng::fluid::network: tank overflow metadata must match tank nodes
```
### Example 2

A tank, with and without overflow.

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
  const auto network = fluid::network(with_tank(400.0, false));
  out.writeln("nodes ", network.nodes(), ", junctions ", network.junctions(), ", reservoirs ", network.reservoirs(), ", tanks ", network.tanks());
  const auto r = fluid::steady(network);
  out.writeln("a steady solve holds the tank at its initial head: ", flat(r.head)[2], " m; flow into it ", flat(r.flow)[1], " m3/s");
  fluid::Schedule schedule;
  schedule.duration = 8.0 * 3600.0;
  schedule.step = 3600.0;
  schedule.pattern_step = 24.0 * 3600.0;
  for (bool overflow : {false, true}) {
    const auto run = fluid::run(fluid::network(with_tank(40.0, overflow)), schedule);
    const auto into = run.flow.select(-1, 1).reshape({run.flow.sizes()[0]}).items<double>();
    out.writeln("area 40 m2, overflow ", overflow, ": level after 8 h ", flat(run.tank_level).back(), " m (max 6 m), flow into the tank ", into.back(), ", spill ", flat(run.spill).back());
  }
}
```

Output

```text
nodes 3, junctions 1, reservoirs 1, tanks 1
a steady solve holds the tank at its initial head: 23.0000 m; flow into it 0.0073 m3/s
area 40 m2, overflow false: level after 8 h 6.0000 m (max 6 m), flow into the tank 0.0000, spill 0.0000
area 40 m2, overflow true: level after 8 h 6.0000 m (max 6 m), flow into the tank 0.0053, spill 0.0053
```
### Example 3

The smallest network, a reservoir and a junction.

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

// a reservoir at 50 m feeding one junction through a 1 km pipe
static fluid::Net one_pipe(double demand = 0.02) {
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
  out.writeln("nodes ", network.nodes(), " (junctions ", network.junctions(), ", reservoirs ", network.reservoirs(), ", tanks ", network.tanks(), ")");
  out.writeln("links ", network.links(), " (pipes ", network.pipes(), ", pumps ", network.pumps(), ", valves ", network.valves(), ")");
  const auto r = fluid::steady(network);
  const double D = 0.30, L = 1000.0, eps = 1.0e-4, q = 0.02, nu = 1.0038e-6, g = 9.80665, pi = 3.14159265358979323846;
  const double v = 4.0 * q / (pi * D * D), Re = v * D / nu;
  const double lg = std::log10(eps / (3.7 * D) + 5.74 / std::pow(Re, 0.9));
  const double loss = 0.25 / (lg * lg) * L / D * v * v / (2.0 * g);
  out.writeln("junction head ", flat(r.head)[1], " m, by hand ", 50.0 - loss, " m");
  out.writeln("pressure ", flat(r.pressure)[1], " Pa gauge, head loss ", flat(r.head_loss)[0], " m, velocity ", flat(r.velocity)[0], " m/s");
}
```

Output

```text
nodes 2 (junctions 1, reservoirs 1, tanks 0)
links 1 (pipes 1, pumps 0, valves 0)
junction head 49.725662 m, by hand 49.725662 m
pressure 437819.413252 Pa gauge, head loss 0.274338 m, velocity 0.282942 m/s
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="ValveKind.html">
    <span class="label">Next</span>
    <span class="title">ValveKind</span>
  </a>
</div>
<!-- apiref:nav:end -->
