<!-- generated from steady.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `fluid::steady`

Solves heads and flows of a network at one instant.

## Signature

```cpp
SteadyResult steady(const Network &network, const Options &options = {});
```

## What It Does

Solves junction heads and link flows as one coupled nonlinear system by Newton's method: pipe energy, pump and valve laws, and junction continuity. The linear solve of each iteration is the batched LU of the whole network, and each lane of a scenario batch is relaxed on its own if its full step does not reduce the residual. The solution is differentiable.

## Parameters

`network`
: The compiled `Network`.

`options`
: The `Options`.

## Returns

A `SteadyResult`.

## Errors

Throws `std::invalid_argument` for invalid options. Throws `std::runtime_error` when the solve does not converge and `fail_on_nonconvergence` is true, when an active pump or an active control valve would need a reverse flow, when an active valve would need to add head, and when closed links leave a junction cut off from every reservoir and tank.

## Interpretation and Use

A solution that is returned satisfies continuity and the link equations to the tolerances. The errors that mention a pump or a valve mean the settings or the curves do not fit the network, not that the solver failed: change the setting or the network.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Heads and flows of a looped network.

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

Tolerances and iteration limits.

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

Options that are refused.

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
  attempt("Hazen-Williams with C = 0", [] {
    auto net = one_pipe();
    net.pipes.roughness = vec({0.0});
    fluid::Options options;
    options.loss = fluid::LossModel::HazenWilliams;
    (void)fluid::steady(fluid::network(net), options);
  });
  attempt("full pressure below the minimum", [] {
    fluid::Options options;
    options.demand_mode = fluid::DemandMode::Pressure;
    options.min_pressure = 20.0;
    options.full_pressure = 10.0;
    (void)fluid::steady(fluid::network(one_pipe()), options);
  });
  attempt("a demand exponent of zero", [] {
    fluid::Options options;
    options.demand_exponent = 0.0;
    (void)fluid::steady(fluid::network(one_pipe()), options);
  });
  attempt("a head tolerance of zero", [] {
    fluid::Options options;
    options.head_tolerance = 0.0;
    (void)fluid::steady(fluid::network(one_pipe()), options);
  });
  attempt("an iteration limit of zero", [] {
    fluid::Options options;
    options.max_iterations = 0;
    (void)fluid::steady(fluid::network(one_pipe()), options);
  });
}
```

Output

```text
Hazen-Williams with C = 0: eng::fluid::steady: Hazen-Williams coefficients must be positive
full pressure below the minimum: eng::fluid::steady: pressure bounds, exponents, tolerances, gradient floor, and iteration limit are invalid
a demand exponent of zero: eng::fluid::steady: pressure bounds, exponents, tolerances, gradient floor, and iteration limit are invalid
a head tolerance of zero: eng::fluid::steady: pressure bounds, exponents, tolerances, gradient floor, and iteration limit are invalid
an iteration limit of zero: eng::fluid::steady: pressure bounds, exponents, tolerances, gradient floor, and iteration limit are invalid
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="SteadyResult.html">
    <span class="label">Previous</span>
    <span class="title">SteadyResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
