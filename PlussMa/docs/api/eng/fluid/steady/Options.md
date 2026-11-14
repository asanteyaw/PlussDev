<!-- generated from Options.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `fluid::Options`

The loss law, demand policy and solver controls of `steady`.

## Signature

```cpp
fluid::Options
```

## What It Does

Controls for `steady`, and for every solve inside `run`, `transport` and `heat`. The tolerances apply to the residuals of the coupled Newton solve.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `loss` | The head-loss law. | DarcyWeisbach |
| `demand_mode` | Fixed or pressure-driven demand. | Fixed |
| `min_pressure` | Pressure head below which no demand is delivered, m. | 0 |
| `full_pressure` | Pressure head above which the whole demand is delivered, m. | 15 |
| `demand_exponent` | The exponent of the Wagner law. | 0.5 |
| `emitter_exponent` | The exponent of every emitter. | 0.5 |
| `head_tolerance` | Largest energy residual of a pipe, m. | 1e-9 |
| `flow_tolerance` | Largest continuity residual of a junction, m³/s. | 1e-11 |
| `min_gradient` | Floor on the head-loss gradient, so that a zero-flow link has a solvable Jacobian. It does not change the reported loss. | 1e-9 |
| `max_iterations` | Newton iteration limit. | 40 |
| `fail_on_nonconvergence` | Throw when the solve does not converge; if false, return the last iterate. | true |

## Returns

A value; validated by `steady`.

## Errors

Non-finite or non-positive tolerances, exponents or iteration limits, and unordered pressure bounds, are rejected.

## Interpretation and Use

Leave the defaults for `Float64`. For `Float32` set `head_tolerance` a little above the single-precision floor, near `1e-5`. With `fail_on_nonconvergence` false always test `converged`: an unfinished iterate reads like an answer.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Iteration limits, tolerances and non-throwing solves.

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
### Example 2

Hazen-Williams, roughness and minor losses.

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
  const double q = 0.05, D = 0.30, L = 1000.0, g = 9.80665, kPi = 3.14159265358979323846;
  const double v = 4.0 * q / (kPi * D * D);
  fluid::Options darcy;
  fluid::Options hazen;
  hazen.loss = fluid::LossModel::HazenWilliams;
  auto pipe = [&](double rough, double minor) {
    auto net = one_pipe(q);
    net.nodes.head = vec({100.0, 90.0});
    net.nodes.elevation = vec({0.0, 0.0});
    net.pipes.roughness = vec({rough});
    net.pipes.minor = vec({minor});
    return fluid::network(net);
  };
  for (double eps : {1e-5, 1e-4, 1e-3}) {
    const auto r = fluid::steady(pipe(eps, 0.0), darcy);
    out.writeln("Darcy-Weisbach, roughness ", eps, " m: friction ", flat(r.friction_factor)[0], ", head loss ", flat(r.head_loss)[0], " m");
  }
  for (double C : {80.0, 130.0, 150.0}) {
    const auto r = fluid::steady(pipe(C, 0.0), hazen);
    const double formula = 10.67 * L * std::pow(q, 1.852) / (std::pow(C, 1.852) * std::pow(D, 4.8704));
    out.writeln("Hazen-Williams, C = ", C, ": head loss ", flat(r.head_loss)[0], ", formula ", formula, ", friction field ", flat(r.friction_factor)[0]);
  }
  const double base = flat(fluid::steady(pipe(1e-4, 0.0), darcy).head_loss)[0];
  out.writeln("minor loss K = 20: head loss ", flat(fluid::steady(pipe(1e-4, 20.0), darcy).head_loss)[0], ", base + K v^2/2g = ", base + 20.0 * v * v / (2.0 * g));
}
```

Output

```text
Darcy-Weisbach, roughness 0.000010 m: friction 0.015677, head loss 1.333124 m
Darcy-Weisbach, roughness 0.000100 m: friction 0.017888, head loss 1.521119 m
Darcy-Weisbach, roughness 0.001000 m: friction 0.027725, head loss 2.357617 m
Hazen-Williams, C = 80.000000: head loss 4.372833, formula 4.372833, friction field 0.000000
Hazen-Williams, C = 130.000000: head loss 1.779354, formula 1.779354, friction field 0.000000
Hazen-Williams, C = 150.000000: head loss 1.365100, formula 1.365100, friction field 0.000000
minor loss K = 20: head loss 2.031336, base + K v^2/2g = 2.031336
```
### Example 3

Pressure-driven demand and emitters.

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
  fluid::Options pressure_driven;
  pressure_driven.demand_mode = fluid::DemandMode::Pressure;
  pressure_driven.min_pressure = 0.0;
  pressure_driven.full_pressure = 15.0;
  pressure_driven.demand_exponent = 0.5;
  for (double demand : {0.2, 0.6, 1.0}) {
    const auto fixed = fluid::steady(fluid::network(one_pipe(demand)));
    const auto driven = fluid::steady(fluid::network(one_pipe(demand)), pressure_driven);
    out.writeln("requested ", demand, ": Fixed delivers ", flat(fixed.demand)[1], " at pressure head ", flat(fixed.head)[1] - 5.0, "; Pressure delivers ", flat(driven.demand)[1], " at pressure head ", flat(driven.head)[1] - 5.0);
  }
  // an emitter: a leak that grows with the square root of the pressure head
  for (double c : {0.0, 0.002, 0.01}) {
    auto net = one_pipe(0.02);
    net.nodes.emitter = vec({0.0, c});
    const auto r = fluid::steady(fluid::network(net));
    const double p = flat(r.head)[1] - 5.0;
    out.writeln("emitter ", c, ": outflow ", flat(r.emitter)[1], ", C sqrt(p) = ", c * std::sqrt(p), ", pipe flow ", flat(r.flow)[0]);
  }
}
```

Output

```text
requested 0.200000: Fixed delivers 0.200000 at pressure head 23.011199; Pressure delivers 0.200000 at pressure head 23.011199
requested 0.600000: Fixed delivers 0.600000 at pressure head -146.321537; Pressure delivers 0.277544 at pressure head 3.209615
requested 1.000000: Fixed delivers 1.000000 at pressure head -482.190058; Pressure delivers 0.284220 at pressure head 1.211716
emitter 0.000000: outflow 0.000000, C sqrt(p) = 0.000000, pipe flow 0.020000
emitter 0.002000: outflow 0.013310, C sqrt(p) = 0.013310, pipe flow 0.033310
emitter 0.010000: outflow 0.063962, C sqrt(p) = 0.063962, pipe flow 0.083962
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="DemandMode.html">
    <span class="label">Previous</span>
    <span class="title">DemandMode</span>
  </a>
  <a class="next-link" href="SteadyResult.html">
    <span class="label">Next</span>
    <span class="title">SteadyResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
