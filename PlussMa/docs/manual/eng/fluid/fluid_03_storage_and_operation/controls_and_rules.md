<!-- generated from controls_and_rules.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Controls and rules

[Back to Fluid 3: Storage and Operation](../fluid_03_storage_and_operation.md)

Nobody stands at a pump station to switch the pump on when the tank is low. A rule does it: when the water level falls below a mark the pump starts, and when it rises above another the pump stops. Rules of this kind decide how a system behaves over a day, and getting them wrong is one of the commonest ways to empty a tank. `Schedule::state` holds rules that watch a level or a pressure, `Schedule::time` holds actions that fire at a given second, and `fluid::run` applies both. This page explains what a rule is and how it is persistent, runs a pump on a two-level rule and checks the cycle against the arithmetic of the tank, applies timed actions including the one that sets a pump speed, and shows the rules that fail.

## The mental model

An **action** is a change to one pump or one valve: `Open`, `Close` or `Set`. It names the `Device` (`Pump` or `Valve`), the `index` among the pumps or the valves, the `Act`, and for `Set` a `value`. `Close` shuts the link, and a closed link carries no flow. `Open` reopens it, and clears any value that an earlier `Set` had latched, so the device returns to what its pattern says. `Set` opens the device and latches a value: a relative speed for a pump, the loss coefficient for a throttle valve, the flow for a flow valve, the pressure head for a pressure valve.

An action is **persistent**: once it has happened it stays in effect until another action changes it. A rule is an action with a trigger, and there are two kinds of trigger:

- A `TimeControl` fires at `at` seconds from the start of the run.
- A `StateControl` watches a node. It fires when the node's value is `Below` or `Above` a `threshold`. The value is the tank level in metres for a tank node, and the pressure head in metres for any other node.

The clock lands on every trigger. A time control fires at its exact second. A tank-level crossing is found from the tank's rate of change, so the event falls on the level itself and not on the next sample. A pressure trigger on a junction depends only on the solved state, and is checked at each hydraulic event.

Two rules with different thresholds make a **hysteresis band**: stop the pump above the high mark and start it below the low one, and between the marks the pump keeps whatever state it had. A single threshold would make the pump chatter on and off around the mark.

## A pump held between two levels

A pump lifts water from a reservoir at `10` m to a junction that draws `0.03` m³/s and is joined to a `200` m² tank through a `0.20` m pipe. The pump delivers far more than the demand, so while it runs the tank fills, and while it is off the tank supplies the demand and drains. Two state controls on the tank node, node `2`, form the band: `Close` the pump above `5` m, `Open` it below `2` m. The program runs a day, finds every switching time from the `open` field, and checks the drain time between marks against the arithmetic of the tank: three metres of level at `0.03` m³/s of demand over `200` m² takes `3 × 200 / 0.03 = 20000` s.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a pump from a low reservoir (10 m) into junction 1, which draws 0.03 m3/s and is joined to a tank by a pipe
static fluid::Net pumped() {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Tank};
  net.nodes.elevation = vec({0.0, 12.0, 20.0});
  net.nodes.demand = vec({0.0, 0.03, 0.0});
  net.nodes.head = vec({10.0, 30.0, 23.0});
  net.tanks.area = vec({200.0});
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
  // the tank is node 2: stop the pump above 5 m, restart it below 2 m
  const fluid::StateControl stop{2, fluid::Sense::Above, 5.0, {fluid::Device::Pump, 0, fluid::Act::Close, 0.0}};
  const fluid::StateControl start{2, fluid::Sense::Below, 2.0, {fluid::Device::Pump, 0, fluid::Act::Open, 0.0}};
  schedule.state = {stop, start};

  const auto r = fluid::run(fluid::network(pumped()), schedule);
  const auto time = flat(r.time), level = flat(r.tank_level), open = flat(r.open);
  out.writeln("samples ", time.size(), ", every state converged: ", r.converged.all().item<bool>());

  // links are numbered pipes first, then pumps: the pump is link 1, so its flag is open[2 * sample + 1]
  std::vector<double> stopped_at, started_at;
  for (std::size_t i = 1; i < time.size(); ++i) {
    if (open[2 * i + 1] < open[2 * (i - 1) + 1]) stopped_at.push_back(time[i]);
    if (open[2 * i + 1] > open[2 * (i - 1) + 1]) started_at.push_back(time[i]);
  }
  for (std::size_t k = 0; k < stopped_at.size(); ++k)
    out.writeln("pump stops at ", stopped_at[k], " s");
  for (std::size_t k = 0; k < started_at.size(); ++k)
    out.writeln("pump starts at ", started_at[k], " s");
  out.writeln("time from the first stop to the next start: ", started_at[0] - stopped_at[0], " s, arithmetic of the tank: ", 3.0 * 200.0 / 0.03, " s");
  for (std::size_t i = 0; i < time.size(); ++i)
    if (time[i] == stopped_at[0] || time[i] == started_at[0])
      out.writeln("at ", time[i], " s the level is ", level[i], " m");
}
```

Output

```text
samples 53, every state converged: true
pump stops at 3605.6130 s
pump stops at 28980.7755 s
pump stops at 54355.9380 s
pump stops at 79731.1005 s
pump starts at 23605.6130 s
pump starts at 48980.7755 s
pump starts at 74355.9380 s
time from the first stop to the next start: 20000.0000 s, arithmetic of the tank: 20000.0000 s
at 3605.6130 s the level is 5.0000 m
at 23605.6130 s the level is 2.0000 m
```

The pump stops when the level reaches `5.0000` m and starts again at `2.0000` m, and each of those switches happens at the sample where the level crosses the mark, not at the nearest half hour: the clock was shortened to land on it. The first stop is at `3605.6` s and the pump restarts `20000.0` s later, exactly the three metres of drain at `0.03` m³/s from the arithmetic. The cycle then repeats on its own: a stop at `28980.8` s, a start at `48980.8` s, and so on, `25375` s per cycle, of which the fill takes `5375` s, about `1.5` hours, and the drain `20000` s, about `5.6`. Between the marks the pump keeps its state. Look at the rules, not the pump, to find why a pump is running.

## Timed actions and the speed latch

A `TimeControl` is the same action at a fixed second. The program below uses three on the same network, with the tank enlarged to `2000` m² so that the level moves slowly and stays inside its limits, and with no state controls. At `4` h the pump is set to `0.7` of its speed, at `8` h it is closed, and at `12` h it is opened. It prints the pump flow and the level at the three moments and just before, and shows what `Open` does to a latched speed: the pump returns to its pattern value, a full speed, not to the `0.7` it was last set to.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static fluid::Net pumped() {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Tank};
  net.nodes.elevation = vec({0.0, 12.0, 20.0});
  net.nodes.demand = vec({0.0, 0.03, 0.0});
  net.nodes.head = vec({10.0, 30.0, 23.0});
  net.tanks.area = vec({2000.0});
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
  schedule.duration = 14.0 * 3600.0;
  schedule.step = 1800.0;
  schedule.pattern_step = 24.0 * 3600.0;
  schedule.time = {
      {4.0 * 3600.0, {fluid::Device::Pump, 0, fluid::Act::Set, 0.7}},
      {8.0 * 3600.0, {fluid::Device::Pump, 0, fluid::Act::Close, 0.0}},
      {12.0 * 3600.0, {fluid::Device::Pump, 0, fluid::Act::Open, 0.0}},
  };
  const auto r = fluid::run(fluid::network(pumped()), schedule);
  const auto time = flat(r.time), level = flat(r.tank_level), open = flat(r.open);
  const auto pump_flow = r.flow.select(-1, 1).reshape({r.flow.sizes()[0]}).items<double>();
  for (std::size_t i = 0; i < time.size(); ++i) {
    const double hours = time[i] / 3600.0;
    if (hours != 0.0 && hours != 3.5 && hours != 4.0 && hours != 4.5 && hours != 7.5 && hours != 8.0 && hours != 11.5 && hours != 12.0 && hours != 12.5 && hours != 14.0) continue;
    out.writeln("t ", hours, " h: pump open ", open[2 * i + 1], ", pump flow ", pump_flow[i], " m3/s, tank level ", level[i], " m");
  }
}
```

Output

```text
t 0.0000 h: pump open 1.0000, pump flow 0.1416 m3/s, tank level 3.0000 m
t 3.5000 h: pump open 1.0000, pump flow 0.1407 m3/s, tank level 3.7007 m
t 4.0000 h: pump open 1.0000, pump flow 0.0862 m3/s, tank level 3.8003 m
t 4.5000 h: pump open 1.0000, pump flow 0.0861 m3/s, tank level 3.8509 m
t 7.5000 h: pump open 1.0000, pump flow 0.0854 m3/s, tank level 4.1523 m
t 8.0000 h: pump open 0.0000, pump flow 0.0000 m3/s, tank level 4.2022 m
t 11.5000 h: pump open 0.0000, pump flow 0.0000 m3/s, tank level 4.0132 m
t 12.0000 h: pump open 1.0000, pump flow 0.1403 m3/s, tank level 3.9862 m
t 12.5000 h: pump open 1.0000, pump flow 0.1401 m3/s, tank level 4.0854 m
t 14.0000 h: pump open 1.0000, pump flow 0.1397 m3/s, tank level 4.3824 m
```

Each action takes effect at the second it names. At `4` h the speed is set to `0.7` and the pump flow falls from `0.1407` m³/s at `3.5` h to `0.0862` m³/s, and the tank, which was rising by about `0.2` m an hour, rises more slowly: `0.0997` m in the half hour before the change and `0.0506` m in the half hour after. At `8` h the pump is closed, its flow is `0.0000` and its `open` flag `0`, and the tank turns to draining, `4.2022` m at `8` h and `4.0132` m at `11.5` h. At `12` h `Open` restores the pump to its full speed, with a flow of `0.1403` m³/s, not to the `0.7` that was set at `4` h. The latch is what `Open` removes. If you want a device to come back to a reduced setting, `Set` it again.

## Decisions

- **Time or state.** Use a time control for a known schedule: an off-peak tariff, a flushing program. Use a state control when the answer depends on the water: a tank level or a pressure. A state control reacts to what actually happens and cannot be wrong about the hour.
- **Always two marks.** Control a pump on a level with two rules and two different thresholds. A single mark makes the pump chatter, and the run refuses rules that cannot settle (see below).
- **Closing and leaving closed.** Every `Close` needs its `Open`, a time later or a level, or the closed device stays closed for the rest of the run. Pair them in your head, and in the schedule.
- **Which node to watch.** Watch the tank for level rules. For a pressure rule watch the junction whose pressure you care about, and expect it to be tested at hydraulic events, not between them.

## Pitfalls

Each of these is a rule that cannot work, and the run says why. In the first the thresholds are the wrong way round: the pump is closed below `5` m and opened above `2` m, so each rule fires while the other is still true. The second names a pump that does not exist. The third gives a time before the run begins. The fourth gives a negative pump speed. The last closes the pump and never reopens it, so the tank drains to its minimum and nothing supplies the demand any more.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <iostream>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

static fluid::Net pumped() {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Tank};
  net.nodes.elevation = vec({0.0, 12.0, 20.0});
  net.nodes.demand = vec({0.0, 0.03, 0.0});
  net.nodes.head = vec({10.0, 30.0, 23.0});
  net.tanks.area = vec({200.0});
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

static void attempt(const char *label, const fluid::Schedule &schedule) {
  try {
    (void)fluid::run(fluid::network(pumped()), schedule);
    std::cout << label << ": ran\n";
  } catch (const std::exception &e) {
    std::cout << label << ": " << e.what() << "\n";
  }
}

int main() {
  fluid::Schedule base;
  base.duration = 24.0 * 3600.0;
  base.step = 1800.0;
  base.pattern_step = 24.0 * 3600.0;
  const fluid::Action close{fluid::Device::Pump, 0, fluid::Act::Close, 0.0};
  const fluid::Action open{fluid::Device::Pump, 0, fluid::Act::Open, 0.0};

  auto inverted = base;
  inverted.state = {{2, fluid::Sense::Below, 5.0, close}, {2, fluid::Sense::Above, 2.0, open}};
  attempt("thresholds the wrong way round", inverted);

  auto missing = base;
  missing.state = {{2, fluid::Sense::Above, 5.0, {fluid::Device::Pump, 3, fluid::Act::Close, 0.0}}};
  attempt("a rule for pump 3 of 1", missing);

  auto early = base;
  early.time = {{-5.0, close}};
  attempt("an action before the run starts", early);

  auto reversed = base;
  reversed.time = {{3600.0, {fluid::Device::Pump, 0, fluid::Act::Set, -1.0}}};
  attempt("a pump speed of -1", reversed);

  auto stuck = base;
  stuck.state = {{2, fluid::Sense::Above, 5.0, close}};
  attempt("close above 5 m and never reopen", stuck);
}
```

Output

```text
thresholds the wrong way round: eng::fluid::run: controls did not settle at a hydraulic event
a rule for pump 3 of 1: eng::fluid::run: state control device index is outside its family
an action before the run starts: eng::fluid::run: timed control lies outside the run interval
a pump speed of -1: eng::fluid::run: timed control setting is invalid
close above 5 m and never reopen: eng::fluid::steady: the hydraulic system is singular: a junction is cut off from every reservoir and tank by closed links
```

The inverted band is refused as `controls did not settle at a hydraulic event`: at a level of `3` m both rules are true at once, close because the level is below `5` and open because it is above `2`, and the run stops rather than flip the pump forever. The next three are refused with a message that names the part of the rule that is invalid. The last runs until the tank drains to its minimum level, which closes the tank's pipe, and the demand junction is left with no path to any source. The solver reports exactly that, as a hydraulic system cut off from every reservoir and tank, in the same way that it refuses a network built that way. The cure is a rule that restarts the pump.

Reference: [`run`](../../../../api/eng/fluid/operation/run.md), [`Schedule`](../../../../api/eng/fluid/operation/Schedule.md), [`StateControl`](../../../../api/eng/fluid/operation/StateControl.md), [`TimeControl`](../../../../api/eng/fluid/operation/TimeControl.md), [`Action`](../../../../api/eng/fluid/operation/Action.md), [`RunResult`](../../../../api/eng/fluid/operation/RunResult.md).

Where next: [Fluid 4: Transport](../fluid_04_transport.md) follows what the water carries, a constituent, its age and its heat, through the flows that the run has found.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tanks_and_patterns.html">
    <span class="label">Previous</span>
    <span class="title">Tanks and patterns</span>
  </a>
</div>
<!-- apiref:nav:end -->
