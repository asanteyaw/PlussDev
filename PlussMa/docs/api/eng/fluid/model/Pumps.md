<!-- generated from Pumps.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `fluid::Pumps`

The curve and speed of the pumps.

## Signature

```cpp
fluid::Pumps
```

## What It Does

Describes the pumps of a `Net`. A pump adds head `s² H0 - R s^(2-n) q^n` between its `from` and `to` nodes at a relative speed `s`, shutoff head `H0`, resistance `R` and exponent `n`. It is one-way: a flow that would run backwards through it is refused.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `from` | The suction node index of each pump. | required |
| `to` | The discharge node index of each pump. | required |
| `shutoff_head` | The head at zero flow and unit speed, m, `[..., pump]`. | required |
| `resistance` | The curve resistance `R`, m / (m³/s)^n, `[..., pump]`. | required |
| `exponent` | The positive curve exponent `n`, `[..., pump]`. | required |
| `speed` | The relative speed, `[..., pump]`. | undefined (1) |

## Returns

A value; validated by `network` and `steady`.

## Errors

An endpoint outside the node range, a pump from a node to itself, mixed precision, and curve values that are not finite or not positive are rejected. `steady` refuses a solution in which an active pump would run backwards.

## Interpretation and Use

Links are numbered pipes first, then pumps, then valves. A pump has no velocity (it reports 0) and its head loss is negative. The flow through a pump is set by the rest of the network: with a demand it is the demand, and between two reservoirs it is where the pump curve meets the system curve.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A pump lifting water between reservoirs.

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

// a pump lifts water from a reservoir at 10 m to a reservoir at 40 m through a 500 m pipe
static fluid::Net transfer(double speed = 1.0) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Reservoir};
  net.nodes.elevation = vec({0.0, 5.0, 0.0});
  net.nodes.demand = vec({0.0, 0.0, 0.0});
  net.nodes.head = vec({10.0, 40.0, 40.0});
  net.pumps.from = {0};
  net.pumps.to = {1};
  net.pumps.shutoff_head = vec({60.0});
  net.pumps.resistance = vec({4000.0});
  net.pumps.exponent = vec({2.0});
  net.pumps.speed = vec({speed});
  net.pipes.from = {1};
  net.pipes.to = {2};
  net.pipes.length = vec({500.0});
  net.pipes.diameter = vec({0.20});
  net.pipes.roughness = vec({1e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const auto network = fluid::network(transfer(1.0));
  out.writeln("pipes ", network.pipes(), ", pumps ", network.pumps(), ", links ", network.links(), " (the pipe is link 0, the pump link 1)");
  const auto r = fluid::steady(network);
  const auto q = flat(r.flow), loss = flat(r.head_loss);
  out.writeln("flow ", q[0], " m3/s; pump gain ", -loss[1], " m, curve at that flow ", 60.0 - 4000.0 * q[1] * q[1], " m; pump velocity ", flat(r.velocity)[1]);
  out.writeln("junction head ", flat(r.head)[1], " m = 10 + gain");
}
```

Output

```text
pipes 1, pumps 1, links 2 (the pipe is link 0, the pump link 1)
flow 0.068941 m3/s; pump gain 40.988681 m, curve at that flow 40.988681 m; pump velocity 0.000000
junction head 50.988681 m = 10 + gain
```
### Example 2

The affinity law and a sensitivity to the speed.

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

// a pump lifts water from a reservoir at 10 m to a reservoir at 40 m through a 500 m pipe
static fluid::Net transfer(double speed = 1.0) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Reservoir};
  net.nodes.elevation = vec({0.0, 5.0, 0.0});
  net.nodes.demand = vec({0.0, 0.0, 0.0});
  net.nodes.head = vec({10.0, 40.0, 40.0});
  net.pumps.from = {0};
  net.pumps.to = {1};
  net.pumps.shutoff_head = vec({60.0});
  net.pumps.resistance = vec({4000.0});
  net.pumps.exponent = vec({2.0});
  net.pumps.speed = vec({speed});
  net.pipes.from = {1};
  net.pipes.to = {2};
  net.pipes.length = vec({500.0});
  net.pipes.diameter = vec({0.20});
  net.pipes.roughness = vec({1e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  for (double s : {0.8, 1.0, 1.2}) {
    const auto r = fluid::steady(fluid::network(transfer(s)));
    out.writeln("speed ", s, ": flow ", flat(r.flow)[0], " m3/s, gain ", -flat(r.head_loss)[1], " m, shutoff head s^2 H0 = ", s * s * 60.0);
  }
  auto net = transfer(1.0);
  net.pumps.speed.tracks_slope_(true);
  const auto tracked = fluid::steady(fluid::network(net));
  tracked.flow.select(-1, 0).reverse();
  out.writeln("d flow / d speed ", flat(net.pumps.speed.sensitivity())[0], " (m3/s per unit of speed)");
  const double h = 1e-6;
  const double up = flat(fluid::steady(fluid::network(transfer(1.0 + h))).flow)[0], down = flat(fluid::steady(fluid::network(transfer(1.0 - h))).flow)[0];
  out.writeln("finite difference ", (up - down) / (2.0 * h));
  attempt("speed 0.5, below the static lift", [] { (void)fluid::steady(fluid::network(transfer(0.5))); });
}
```

Output

```text
speed 0.800000: flow 0.036182 m3/s, gain 33.163338 m, shutoff head s^2 H0 = 38.400000
speed 1.000000: flow 0.068941 m3/s, gain 40.988681 m, shutoff head s^2 H0 = 60.000000
speed 1.200000: flow 0.094797 m3/s, gain 50.454030 m, shutoff head s^2 H0 = 86.400000
d flow / d speed 0.139283 (m3/s per unit of speed)
finite difference 0.139283
speed 0.5, below the static lift: eng::fluid::steady: an active pump requires reverse flow
```
### Example 3

A pump switched by tank level.

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
  <a class="prev-link" href="Pipes.html">
    <span class="label">Previous</span>
    <span class="title">Pipes</span>
  </a>
  <a class="next-link" href="Valves.html">
    <span class="label">Next</span>
    <span class="title">Valves</span>
  </a>
</div>
<!-- apiref:nav:end -->
