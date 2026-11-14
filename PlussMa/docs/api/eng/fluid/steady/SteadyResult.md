<!-- generated from SteadyResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `fluid::SteadyResult`

Heads, pressures, flows and residuals of a steady solve.

## Signature

```cpp
fluid::SteadyResult
```

## What It Does

The result of `steady`. Arrays keep the leading scenario axes; link arrays follow the order pipes, pumps, valves.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `head` | Node head, m, `[..., node]`. |  |
| `pressure` | Gauge pressure, Pa, `[..., node]`: `ρ g (head - elevation)`. |  |
| `flow` | Link flow, m³/s, `[..., link]`; negative means against the declared direction. |  |
| `velocity` | Link velocity, m/s, `[..., link]`; 0 for a pump. |  |
| `head_loss` | Head lost across a link, m, `[..., link]`; negative across a pump. |  |
| `friction_factor` | Darcy friction factor, `[..., link]`; 0 for non-Darcy links. |  |
| `balance` | Inflow minus outflow minus demand, m³/s, `[..., node]`; at a reservoir it is minus the supply. |  |
| `energy_residual` | The residual of each link equation, `[..., link]`. |  |
| `converged` | Whether each scenario met the tolerances, `[...]`. |  |
| `demand` | Delivered ordinary demand, m³/s, `[..., node]`. |  |
| `emitter` | Emitter outflow, m³/s, `[..., node]`. |  |
| `iterations` | The Newton iterations taken. |  |

## Returns

Returned by `steady`.

## Errors

Reading a field does not solve again.

## Interpretation and Use

Check `converged` when `fail_on_nonconvergence` is false. `balance` is zero at every junction; the residual arrays show how well the equations hold. Under a fixed demand also read `pressure`: a delivered demand can come with a negative pressure.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Every field of a looped network.

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

// a reservoir (node 0) and four junctions joined by six pipes that form two loops
static fluid::Net looped() {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Junction,
                    fluid::NodeKind::Junction, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 10.0, 12.0, 8.0, 15.0});
  net.nodes.demand = vec({0.0, 0.010, 0.015, 0.012, 0.008});
  net.nodes.head = vec({60.0, 55.0, 55.0, 55.0, 55.0});
  net.pipes.from = {0, 1, 1, 2, 3, 2};
  net.pipes.to = {1, 2, 3, 4, 4, 3};
  net.pipes.length = vec({800.0, 500.0, 600.0, 450.0, 550.0, 300.0});
  net.pipes.diameter = vec({0.30, 0.20, 0.20, 0.15, 0.15, 0.10});
  net.pipes.roughness = vec({1e-4, 1e-4, 1e-4, 1e-4, 1e-4, 1e-4});
  net.pipes.minor = vec({0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const auto r = fluid::steady(fluid::network(looped()));
  out.writeln("head              ", flat(r.head));
  out.writeln("pressure (kPa)    ", (r.pressure / 1000.0).reshape({r.pressure.numel()}).items<double>());
  out.writeln("flow              ", flat(r.flow));
  out.writeln("velocity          ", flat(r.velocity));
  out.writeln("head loss         ", flat(r.head_loss));
  out.writeln("friction factor   ", flat(r.friction_factor));
  out.writeln("balance           ", flat(r.balance));
  out.writeln("demand delivered  ", flat(r.demand));
  out.writeln("emitter outflow   ", flat(r.emitter));
  out.writeln("converged ", flat(r.converged)[0], " in ", r.iterations, " iterations");
  out.writeln("largest energy residual ", r.energy_residual.abs().max().item<double>(), " m");
}
```

Output

```text
head              [60.000000, 59.002957, 58.118057, 58.146721, 57.917692]
pressure (kPa)    [587.339882, 479.689849, 451.449572, 490.886153, 420.121206]
flow              [0.045000, 0.018502, 0.016498, 0.004074, 0.003926, -0.000572]
velocity          [0.636620, 0.588934, 0.525151, 0.230515, 0.222192, -0.072783]
head loss         [0.997043, 0.884900, 0.856236, 0.200365, 0.229029, -0.028664]
friction factor   [0.018094, 0.020016, 0.020298, 0.024652, 0.024815, 0.035375]
balance           [-0.045000, -0.000000, 0.000000, -0.000000, 0.000000]
demand delivered  [0.000000, 0.010000, 0.015000, 0.012000, 0.008000]
emitter outflow   [0.000000, 0.000000, 0.000000, 0.000000, 0.000000]
converged 1.000000 in 3 iterations
largest energy residual 0.000000 m
```
### Example 2

Residuals of an unfinished solve.

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

// a reservoir (node 0) and four junctions joined by six pipes that form two loops
static fluid::Net looped() {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Junction,
                    fluid::NodeKind::Junction, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 10.0, 12.0, 8.0, 15.0});
  net.nodes.demand = vec({0.0, 0.010, 0.015, 0.012, 0.008});
  net.nodes.head = vec({60.0, 55.0, 55.0, 55.0, 55.0});
  net.pipes.from = {0, 1, 1, 2, 3, 2};
  net.pipes.to = {1, 2, 3, 4, 4, 3};
  net.pipes.length = vec({800.0, 500.0, 600.0, 450.0, 550.0, 300.0});
  net.pipes.diameter = vec({0.30, 0.20, 0.20, 0.15, 0.15, 0.10});
  net.pipes.roughness = vec({1e-4, 1e-4, 1e-4, 1e-4, 1e-4, 1e-4});
  net.pipes.minor = vec({0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const auto network = fluid::network(looped());
  for (std::int64_t limit : {1, 2, 3}) {
    fluid::Options options;
    options.max_iterations = limit;
    options.fail_on_nonconvergence = false;
    const auto s = fluid::steady(network, options);
    out.writeln("max_iterations ", limit, ": converged ", flat(s.converged)[0], ", iterations ", s.iterations, ", largest energy residual ", s.energy_residual.abs().max().item<double>());
  }
  fluid::Options strict;
  strict.max_iterations = 1;
  attempt("max_iterations 1, failing on nonconvergence", [&] { (void)fluid::steady(network, strict); });
  for (double tolerance : {1e-3, 1e-6, 1e-12}) {
    fluid::Options options;
    options.head_tolerance = tolerance;
    options.flow_tolerance = tolerance * 1e-2;
    out.writeln("head_tolerance 1e-", int(-std::log10(tolerance) + 0.5), ": iterations ", fluid::steady(network, options).iterations);
  }
}
```

Output

```text
max_iterations 1: converged 0.000000, iterations 1, largest energy residual 0.002334
max_iterations 2: converged 0.000000, iterations 2, largest energy residual 0.000004
max_iterations 3: converged 1.000000, iterations 3, largest energy residual 0.000000
max_iterations 1, failing on nonconvergence: eng::fluid::steady: coupled hydraulic Newton solve did not converge
head_tolerance 1e-3: iterations 2
head_tolerance 1e-6: iterations 3
head_tolerance 1e-12: iterations 4
```
### Example 3

A pump gain and a pump velocity.

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

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Options.html">
    <span class="label">Previous</span>
    <span class="title">Options</span>
  </a>
  <a class="next-link" href="steady.html">
    <span class="label">Next</span>
    <span class="title">steady</span>
  </a>
</div>
<!-- apiref:nav:end -->
