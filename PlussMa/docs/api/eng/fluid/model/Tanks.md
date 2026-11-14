<!-- generated from Tanks.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `fluid::Tanks`

The cross-section, level limits and overflow behaviour of the tanks.

## Signature

```cpp
fluid::Tanks
```

## What It Does

Describes the tanks of a `Net`, one entry per node of kind `Tank`, in the order of the nodes. A tank is a cylinder: its area is constant, and a level-volume curve is not represented.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `area` | Cross-section, m², `[..., tank]`. | required if there are tanks |
| `min_level` | Lowest level above the bottom, m, `[..., tank]`. | required if there are tanks |
| `max_level` | Highest level above the bottom, m, `[..., tank]`. | required if there are tanks |
| `overflow` | Per tank, true to spill the excess at the maximum level, false to close the inlet. | required if there are tanks |

## Returns

A value; validated by `network` and `run`.

## Errors

Areas that are not positive, limits that are out of order, an initial level outside the limits, and a count that does not match the number of tank nodes are rejected.

## Interpretation and Use

The initial level of a tank is `Nodes::head - Nodes::elevation`. At its maximum level a tank with `overflow` false closes its inlet; with `overflow` true it holds the level and reports the excess as `RunResult::spill`. At its minimum level it closes its outlet.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

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
### Example 2

A day of demand patterns.

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
  fluid::Schedule schedule;
  schedule.duration = 12.0 * 3600.0;
  schedule.step = 1800.0;
  schedule.pattern_step = 3.0 * 3600.0;
  schedule.demand = pluss::series::make(std::vector<double>{0, 0.5, 0, 0, 1.0, 0, 0, 2.0, 0, 0, 3.0, 0}, {4, 3});
  const auto r = fluid::run(fluid::network(with_tank(400.0, false)), schedule);
  const auto time = flat(r.time), level = flat(r.tank_level);
  const auto into = r.flow.select(-1, 1).reshape({r.flow.sizes()[0]}).items<double>();
  const auto delivered = r.demand.select(-1, 1).reshape({r.demand.sizes()[0]}).items<double>();
  out.writeln("samples ", time.size(), ", every state converged ", r.converged.all().item<bool>());
  for (std::size_t i = 0; i < time.size(); i += 6)
    out.writeln("t ", time[i] / 3600.0, " h: demand ", delivered[i], ", flow into the tank ", into[i], ", level ", level[i]);
  double volume = 0.0;
  for (std::size_t i = 0; i + 1 < time.size(); ++i) volume += into[i] * (time[i + 1] - time[i]);
  out.writeln("volume into the tank ", volume, " m3, area * change of level ", 400.0 * (level.back() - level.front()), " m3");
}
```

Output

```text
samples 25, every state converged true
t 0.0000 h: demand 0.0200, flow into the tank 0.0265, level 3.0000
t 3.0000 h: demand 0.0400, flow into the tank 0.0068, level 3.7093
t 6.0000 h: demand 0.0800, flow into the tank -0.0319, level 3.8925
t 9.0000 h: demand 0.1200, flow into the tank -0.0674, level 3.0368
t 12.0000 h: demand 0.0200, flow into the tank 0.0276, level 1.2274
volume into the tank -709.0545 m3, area * change of level -709.0545 m3
```
### Example 3

A tank that starts above its maximum level.

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

// a pump from a low reservoir (10 m) into junction 1, which draws 0.03 m3/s and is joined to a 200 m2 tank by a pipe
static fluid::Net pumped(double area = 200.0) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Tank};
  net.nodes.elevation = vec({0.0, 12.0, 20.0});
  net.nodes.demand = vec({0.0, 0.03, 0.0});
  net.nodes.head = vec({10.0, 30.0, 23.0});
  net.tanks.area = vec({area});
  net.tanks.min_level = vec({0.5});
  net.tanks.max_level = vec({6.0});
  net.tanks.overflow = {false};
  net.pumps.from = {0};
  net.pumps.to = {1};
  net.pumps.shutoff_head = vec({60.0});
  net.pumps.resistance = vec({1500.0});
  net.pumps.exponent = vec({2.0});
  net.pumps.speed = vec({1.0});
  net.pipes.from = {1};
  net.pipes.to = {2};
  net.pipes.length = vec({300.0});
  net.pipes.diameter = vec({0.20});
  net.pipes.roughness = vec({1e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  fluid::Schedule base;
  base.duration = 24.0 * 3600.0;
  base.step = 1800.0;
  base.pattern_step = 24.0 * 3600.0;
  const fluid::Action close{fluid::Device::Pump, 0, fluid::Act::Close, 0.0};
  const fluid::Action open{fluid::Device::Pump, 0, fluid::Act::Open, 0.0};
  attempt("a tank that starts above its maximum level", [&] { (void)fluid::run(fluid::network(with_tank(400.0, false, 29.0)), base); });
  attempt("a demand pattern with two columns for three nodes", [&] {
    auto s = base;
    s.demand = pluss::series::make(std::vector<double>{0.0, 1.0}, {1, 2});
    (void)fluid::run(fluid::network(with_tank()), s);
  });
  attempt("a negative demand multiplier", [&] {
    auto s = base;
    s.demand = pluss::series::make(std::vector<double>{0.0, -1.0, 0.0}, {1, 3});
    (void)fluid::run(fluid::network(with_tank()), s);
  });
  attempt("thresholds the wrong way round", [&] {
    auto s = base;
    s.state = {{2, fluid::Sense::Below, 5.0, close}, {2, fluid::Sense::Above, 2.0, open}};
    (void)fluid::run(fluid::network(pumped()), s);
  });
  attempt("a rule for pump 3 of 1", [&] {
    auto s = base;
    s.state = {{2, fluid::Sense::Above, 5.0, {fluid::Device::Pump, 3, fluid::Act::Close, 0.0}}};
    (void)fluid::run(fluid::network(pumped()), s);
  });
  attempt("an action before the run starts", [&] {
    auto s = base;
    s.time = {{-5.0, close}};
    (void)fluid::run(fluid::network(pumped()), s);
  });
  attempt("a pump speed of -1", [&] {
    auto s = base;
    s.time = {{3600.0, {fluid::Device::Pump, 0, fluid::Act::Set, -1.0}}};
    (void)fluid::run(fluid::network(pumped()), s);
  });
  attempt("close above 5 m and never reopen", [&] {
    auto s = base;
    s.state = {{2, fluid::Sense::Above, 5.0, close}};
    (void)fluid::run(fluid::network(pumped()), s);
  });
}
```

Output

```text
a tank that starts above its maximum level: eng::fluid::network: initial tank level must be within bounds
a demand pattern with two columns for three nodes: eng::fluid::run: demand pattern must be real floating [period, element]
a negative demand multiplier: eng::fluid::run: demand pattern must be nonnegative
thresholds the wrong way round: eng::fluid::run: controls did not settle at a hydraulic event
a rule for pump 3 of 1: eng::fluid::run: state control device index is outside its family
an action before the run starts: eng::fluid::run: timed control lies outside the run interval
a pump speed of -1: eng::fluid::run: timed control setting is invalid
close above 5 m and never reopen: eng::fluid::steady: the hydraulic system is singular: a junction is cut off from every reservoir and tank by closed links
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Nodes.html">
    <span class="label">Previous</span>
    <span class="title">Nodes</span>
  </a>
  <a class="next-link" href="Pipes.html">
    <span class="label">Next</span>
    <span class="title">Pipes</span>
  </a>
</div>
<!-- apiref:nav:end -->
