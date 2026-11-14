<!-- generated from LossModel.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `fluid::LossModel`

The head-loss law of the pipes.

## Signature

```cpp
fluid::LossModel
```

## What It Does

Chooses the law that gives the head a pipe loses at a flow. It is one setting for the whole network, and it changes the meaning of `Pipes::roughness`.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `DarcyWeisbach` | `h = (f L/D + K) v ∣v∣ / (2 g)`. The friction factor `f` is `64/Re` below `Re = 2000`, a cubic interpolation to `4000`, and the Swamee-Jain relation above. `roughness` is an absolute roughness in m. |  |
| `HazenWilliams` | `h = 10.67 L ∣q∣^0.852 q / (C^1.852 D^4.8704) + K v ∣v∣ / (2 g)`. `roughness` is the coefficient `C`. The friction factor is reported as 0. |  |

## Returns

An enumeration; `Options::loss`.

## Errors

`steady` rejects Hazen-Williams coefficients that are not positive.

## Interpretation and Use

Use `DarcyWeisbach` by default; it holds at every Reynolds number and for any liquid. Use `HazenWilliams` when the data are `C` coefficients and the flow is turbulent water. A roughness of `1e-4` is a fine Darcy roughness and an absurd `C`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Both laws against their formulas.

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
### Example 2

Coefficients that are refused.

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
### Example 3

A solve with each law.

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

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="DemandMode.html">
    <span class="label">Next</span>
    <span class="title">DemandMode</span>
  </a>
</div>
<!-- apiref:nav:end -->
