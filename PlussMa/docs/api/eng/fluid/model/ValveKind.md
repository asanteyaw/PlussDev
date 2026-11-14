<!-- generated from ValveKind.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `fluid::ValveKind`

The four control valves and the equation each enforces.

## Signature

```cpp
fluid::ValveKind
```

## What It Does

Chooses the equation that replaces the ordinary head-loss law of a valve link. `Throttle` adds a known loss, `Flow` fixes the flow through the valve, `Reduce` fixes the pressure head downstream and `Sustain` fixes it upstream.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `Throttle` | `h_from - h_to = K v ∣v∣ / (2 g)`, with `K` from `Valves::minor`. |  |
| `Flow` | The flow through the valve equals `Valves::setting`, in m³/s. |  |
| `Reduce` | The head downstream equals the downstream elevation plus `setting`, a pressure head in m. |  |
| `Sustain` | The head upstream equals the upstream elevation plus `setting`, a pressure head in m. |  |

## Returns

An enumeration; one value per valve in `Valves::kind`.

## Errors

A pressure valve that does not regulate a junction is rejected by `network`. A setting that the network cannot satisfy is rejected by `steady`: it needs a valve that adds head or a flow that runs backwards.

## Interpretation and Use

A valve never adds head. Use `Reduce` on the only path into the zone it protects, `Sustain` on a junction that has another way to be supplied, and `Flow` only for a rate that the head difference can pass.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

All four valve kinds solved against their equations.

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

// reservoir 0 (80 m) feeds junction 1 through one pipe, and junction 2 through another unless second_pipe is false; a valve joins 1 -> 2
static fluid::Net with_valve(fluid::ValveKind kind, double setting, double K = 0.0, bool second_pipe = true) {
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
  net.valves.minor = vec({K});
  net.valves.setting = vec({setting});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  struct Case { const char *label; fluid::ValveKind kind; double setting; double K; bool second; };
  const Case cases[] = {
      {"Throttle K = 100", fluid::ValveKind::Throttle, 0.0, 100.0, true},
      {"Flow 0.005 m3/s", fluid::ValveKind::Flow, 0.005, 0.0, true},
      {"Reduce 40 m (valve is the only feed)", fluid::ValveKind::Reduce, 40.0, 0.0, false},
      {"Sustain 68 m", fluid::ValveKind::Sustain, 68.0, 0.0, true},
  };
  for (const auto &c : cases) {
    const auto net = fluid::network(with_valve(c.kind, c.setting, c.K, c.second));
    const auto r = fluid::steady(net);
    out.writeln(c.label, ": links ", net.links(), ", valve flow ", flat(r.flow).back(), ", head across the valve ", flat(r.head_loss).back(), ", junction heads ", flat(r.head)[1], " ", flat(r.head)[2]);
  }
}
```

Output

```text
Throttle K = 100: links 3, valve flow 0.00845, head across the valve 0.36874, junction heads 77.60244 77.23370
Flow 0.005 m3/s: links 3, valve flow 0.00500, head across the valve 2.65961, junction heads 78.12469 75.46508
Reduce 40 m (valve is the only feed): links 2, valve flow 0.02000, head across the valve 30.39810, junction heads 75.39810 45.00000
Sustain 68 m: links 3, valve flow 0.00586, head across the valve 2.05304, junction heads 78.00000 75.94696
```
### Example 2

A throttle valve against `K v²/2g`.

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

// reservoir 0 (80 m) feeds junction 1 through one pipe, and junction 2 through another unless second_pipe is false; a valve joins 1 -> 2
static fluid::Net with_valve(fluid::ValveKind kind, double setting, double K = 0.0, bool second_pipe = true) {
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
    const double q = flat(r.flow)[2], v = 4.0 * q / (kPi * 0.2 * 0.2);
    out.writeln("K = ", K, ": valve flow ", q, ", head across it ", flat(r.head_loss)[2], ", K v^2/2g = ", K * v * v / (2.0 * g));
  }
}
```

Output

```text
K = 0.00000: valve flow 0.00906, head across it 0.00000, K v^2/2g = 0.00000
K = 10.00000: valve flow 0.00899, head across it 0.04175, K v^2/2g = 0.04175
K = 100.00000: valve flow 0.00845, head across it 0.36874, K v^2/2g = 0.36874
K = 1000.00000: valve flow 0.00608, head across it 1.90664, K v^2/2g = 1.90664
```
### Example 3

Settings the network cannot satisfy.

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

// reservoir 0 (80 m) feeds junction 1 through one pipe, and junction 2 through another unless second_pipe is false; a valve joins 1 -> 2
static fluid::Net with_valve(fluid::ValveKind kind, double setting, double K = 0.0, bool second_pipe = true) {
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
  net.valves.minor = vec({K});
  net.valves.setting = vec({setting});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  attempt("a flow valve set to 0.02 (it can pass 0.009)", [] { (void)fluid::steady(fluid::network(with_valve(fluid::ValveKind::Flow, 0.02))); });
  attempt("a sustaining valve set to 60 m (the junction has 67.5 m)", [] { (void)fluid::steady(fluid::network(with_valve(fluid::ValveKind::Sustain, 60.0))); });
  attempt("a sustaining valve set to 69 m", [] { (void)fluid::steady(fluid::network(with_valve(fluid::ValveKind::Sustain, 69.0))); });
  attempt("a reducing valve beside a second pipe", [] { (void)fluid::steady(fluid::network(with_valve(fluid::ValveKind::Reduce, 40.0))); });
  attempt("a valve that is the only feed, reducing to 40 m", [] { (void)fluid::steady(fluid::network(with_valve(fluid::ValveKind::Reduce, 40.0, 0.0, false))); });
}
```

Output

```text
a flow valve set to 0.02 (it can pass 0.009): eng::fluid::steady: an active control valve cannot add head
a sustaining valve set to 60 m (the junction has 67.5 m): eng::fluid::steady: an active control valve cannot add head
a sustaining valve set to 69 m: eng::fluid::steady: an active control valve requires reverse flow
a reducing valve beside a second pipe: eng::fluid::steady: an active control valve requires reverse flow
a valve that is the only feed, reducing to 40 m: accepted
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="NodeKind.html">
    <span class="label">Previous</span>
    <span class="title">NodeKind</span>
  </a>
  <a class="next-link" href="Nodes.html">
    <span class="label">Next</span>
    <span class="title">Nodes</span>
  </a>
</div>
<!-- apiref:nav:end -->
