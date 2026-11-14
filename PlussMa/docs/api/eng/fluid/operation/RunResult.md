<!-- generated from RunResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `fluid::RunResult`

The state at every hydraulic event of a run.

## Signature

```cpp
fluid::RunResult
```

## What It Does

The result of `run`. Every steady field is stacked along a leading sample axis. The samples include `t = 0`, every shortened event and the final time.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `time` | Sample times, s, `[sample]`. |  |
| `head` | Node head, m, `[sample, ..., node]`. |  |
| `pressure` | Gauge pressure, Pa, `[sample, ..., node]`. |  |
| `flow` | Link flow, m³/s, `[sample, ..., link]`. |  |
| `velocity` | Link velocity, m/s, `[sample, ..., link]`. |  |
| `head_loss` | Head loss, m, `[sample, ..., link]`. |  |
| `friction_factor` | Darcy friction factor, `[sample, ..., link]`. |  |
| `balance` | Node continuity residual, `[sample, ..., node]`. |  |
| `energy_residual` | Link equation residual, `[sample, ..., link]`. |  |
| `demand` | Delivered demand, `[sample, ..., node]`. |  |
| `emitter` | Emitter outflow, `[sample, ..., node]`. |  |
| `tank_level` | Level above the bottom, m, `[sample, ..., tank]`. |  |
| `spill` | Overflow discharge, m³/s, `[sample, ..., tank]`. |  |
| `open` | Whether each link is open, `[sample, ..., link]`. |  |
| `converged` | Whether each solve met its tolerances, `[sample, ...]`. |  |
| `iterations` | Newton iterations of each solve, `[sample]`. |  |

## Returns

Returned by `run`.

## Errors

Reading a field does not run again.

## Interpretation and Use

A sample holds the state at its time, and its flows hold until the next sample. Use `time` to index the first axis. Volume conservation is exact: the area times the change of a tank level equals the integral of its inflow minus its spill.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Levels and flows over a day.

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

Every field, with a spilling tank.

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
  schedule.duration = 8.0 * 3600.0;
  schedule.step = 3600.0;
  schedule.pattern_step = 8.0 * 3600.0;
  const auto r = fluid::run(fluid::network(with_tank(40.0, true)), schedule);
  out.writeln("samples ", r.time.sizes(), "; head ", r.head.sizes(), ", flow ", r.flow.sizes(), ", tank_level ", r.tank_level.sizes(), ", open ", r.open.sizes(), ", converged ", r.converged.sizes());
  const auto t = flat(r.time), level = flat(r.tank_level), spill = flat(r.spill), iterations = flat(r.iterations);
  for (std::size_t i = 0; i < t.size(); ++i)
    out.writeln("t ", t[i], " s: level ", level[i], ", spill ", spill[i], ", Newton iterations ", iterations[i]);
  const auto supply = r.flow.select(-1, 0).reshape({r.flow.sizes()[0]}).items<double>();
  const auto into = r.flow.select(-1, 1).reshape({r.flow.sizes()[0]}).items<double>();
  out.writeln("at the end: supply ", supply.back(), " = demand 0.04 + tank pipe ", into.back(), "; the tank pipe leaves as spill ", spill.back());
  out.writeln("energy residual at the last sample ", r.energy_residual.select(0, r.energy_residual.sizes()[0] - 1).abs().max().item<double>());
}
```

Output

```text
samples [10]; head [10, 3], flow [10, 2], tank_level [10, 1], open [10, 2], converged [10]
t 0.0000 s: level 3.0000, spill 0.0000, Newton iterations 5.0000
t 3600.0000 s: level 3.6565, spill 0.0000, Newton iterations 3.0000
t 7200.0000 s: level 4.2749, spill 0.0000, Newton iterations 3.0000
t 10800.0000 s: level 4.8568, spill 0.0000, Newton iterations 3.0000
t 14400.0000 s: level 5.4043, spill 0.0000, Newton iterations 3.0000
t 18000.0000 s: level 5.9190, spill 0.0000, Newton iterations 3.0000
t 18602.8532 s: level 6.0000, spill 0.0053, Newton iterations 2.0000
t 22202.8532 s: level 6.0000, spill 0.0053, Newton iterations 0.0000
t 25802.8532 s: level 6.0000, spill 0.0053, Newton iterations 0.0000
t 28800.0000 s: level 6.0000, spill 0.0053, Newton iterations 0.0000
at the end: supply 0.0453 = demand 0.04 + tank pipe 0.0053; the tank pipe leaves as spill 0.0053
energy residual at the last sample 0.0000
```
### Example 3

The `open` flag of a switched pump.

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

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Schedule.html">
    <span class="label">Previous</span>
    <span class="title">Schedule</span>
  </a>
  <a class="next-link" href="run.html">
    <span class="label">Next</span>
    <span class="title">run</span>
  </a>
</div>
<!-- apiref:nav:end -->
