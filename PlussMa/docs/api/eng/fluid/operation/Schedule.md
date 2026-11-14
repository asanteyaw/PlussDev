<!-- generated from Schedule.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `fluid::Schedule`

The clock, patterns and controls of a run.

## Signature

```cpp
fluid::Schedule
```

## What It Does

Describes a run: its length, the longest hydraulic step, the period of the cyclic patterns, the patterns themselves and the controls. The clock lands on every pattern boundary, timed action, control crossing and tank limit.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `duration` | Length of the run, s. | required |
| `step` | Longest hydraulic step, s. | 3600 |
| `pattern_step` | Length of one pattern period, s. | 3600 |
| `demand` | `[period, node]` multipliers on the base demands, cyclic. | undefined (constant) |
| `pump` | `[period, pump]` multipliers on the relative speeds, cyclic. | undefined (constant) |
| `valve` | `[period, valve]` absolute valve settings, cyclic. | undefined (constant) |
| `time` | The `TimeControl`s. | none |
| `state` | The `StateControl`s. | none |

## Returns

A value; validated by `run`, `transport` and `heat`.

## Errors

A non-positive duration or step, a pattern of the wrong width or with negative values, and invalid controls are rejected by `run`.

## Interpretation and Use

`step` is a ceiling, so a long step costs accuracy only in the tank balance, never in the timing. Rows are periods; columns follow the order of nodes, pumps and valves. For `transport` and `heat` the hydraulic step also cuts the quality step.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

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
### Example 2

Level and time controls.

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
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  fluid::Schedule schedule;
  schedule.duration = 24.0 * 3600.0;
  schedule.step = 1800.0;
  schedule.pattern_step = 24.0 * 3600.0;
  const fluid::StateControl stop{2, fluid::Sense::Above, 5.0, {fluid::Device::Pump, 0, fluid::Act::Close, 0.0}};
  const fluid::StateControl start{2, fluid::Sense::Below, 2.0, {fluid::Device::Pump, 0, fluid::Act::Open, 0.0}};
  schedule.state = {stop, start};
  const auto r = fluid::run(fluid::network(pumped()), schedule);
  const auto time = flat(r.time), level = flat(r.tank_level), open = flat(r.open);
  std::vector<double> stops, starts;
  for (std::size_t i = 1; i < time.size(); ++i) {
    if (open[2 * i + 1] < open[2 * (i - 1) + 1]) stops.push_back(time[i]);
    if (open[2 * i + 1] > open[2 * (i - 1) + 1]) starts.push_back(time[i]);
  }
  out.writeln("pump stops at ", stops[0], " s, starts at ", starts[0], " s: ", starts[0] - stops[0], " s of drain; arithmetic ", 3.0 * 200.0 / 0.03, " s");
  out.writeln("stops at ", stops, " s");
  out.writeln("starts at ", starts, " s");
}
```

Output

```text
pump stops at 3605.6130 s, starts at 23605.6130 s: 20000.0000 s of drain; arithmetic 20000.0000 s
stops at [3605.6130, 28980.7755, 54355.9380, 79731.1005] s
starts at [23605.6130, 48980.7755, 74355.9380] s
```
### Example 3

Schedules that are refused.

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
  <a class="prev-link" href="StateControl.html">
    <span class="label">Previous</span>
    <span class="title">StateControl</span>
  </a>
  <a class="next-link" href="RunResult.html">
    <span class="label">Next</span>
    <span class="title">RunResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
