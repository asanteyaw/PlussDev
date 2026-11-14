<!-- generated from DispatchResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::DispatchResult`

Generation, flows, post-outage flows and solver diagnostics of `dispatch`.

## Signature

```cpp
power::DispatchResult
```

## What It Does

The result of `dispatch`. Arrays keep the leading scenario axes of the grid.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `generation` | The optimal active output of each generator, `[..., generator]`. |  |
| `angle` | The DC angles, `[..., bus]`. |  |
| `injection` | The net active injection at each bus, `[..., bus]`. |  |
| `flow` | The DC flow on each line, `[..., line]`. |  |
| `post_flow` | With `secure` true, the flows after each outage, `[..., outage line, monitored line]`; undefined otherwise. |  |
| `outage_valid` | With `secure` true, whether each outage is valid, `[..., outage line]`; undefined otherwise. |  |
| `objective` | The total cost from the quadratic and linear terms, `[...]`. |  |
| `converged` | True when the solver met its tolerances. |  |
| `iterations` | Solver iterations used. |  |
| `primal_residual` | The violation of the constraints at the returned point. |  |
| `dual_residual` | The optimality residual. |  |

## Returns

Returned by `dispatch`.

## Errors

Reading a field does not solve again.

## Interpretation and Use

Check `converged` first. An infeasible dispatch does not throw: it returns with `converged` false and a primal residual that says by how much the constraints are violated.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Every field against the hand optimum.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <limits>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a ring of three buses: generators at buses 0 and 1, a load of 1.5 per unit at bus 2.
// Lines: 0 joins 0-1 (x = 0.1), 1 joins 1-2 (x = 0.2), 2 joins 0-2 (x = 0.3).
static power::Net ring() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, 0.0, -1.5});
  net.buses.q = vec({0.0, 0.0, -0.3});
  net.buses.vm = vec({1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.gens.bus = {0, 1};
  net.gens.p = vec({0.5, 1.0});    // starting values: the dispatch chooses them
  net.gens.q = vec({0.0, 0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.03});
  net.lines.x = vec({0.1, 0.2, 0.3});
  return net;
}

// generator 0: cost 5 g^2 + 10 g, generator 1: cost 8 g^2 + 12 g, each between 0 and 2 per unit
static power::GenCost costs() {
  power::GenCost c;
  c.lower = vec({0.0, 0.0});
  c.upper = vec({2.0, 2.0});
  c.linear = vec({10.0, 12.0});
  c.quadratic = vec({5.0, 8.0});
  return c;
}

// a tight tolerance for the quadratic-programming solver
static power::DispatchOptions tight() {
  power::DispatchOptions options;
  options.qp.abs_tol = 1e-10;
  options.qp.rel_tol = 1e-10;
  options.qp.max_iters = 50000;
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8).with_sep(""));
  const power::Grid grid = power::make(ring());
  const auto ratings = vec({10.0, 10.0, 10.0});

  const auto rough = power::dispatch(grid, costs(), ratings);
  out.writeln("default solver: generation ", flat(rough.generation), ", cost ", flat(rough.objective), ", converged ", rough.converged, " in ", rough.iterations, " iterations");
  const auto exact = power::dispatch(grid, costs(), ratings, tight());
  out.writeln("tight solver:   generation ", flat(exact.generation), ", cost ", flat(exact.objective), ", converged ", exact.converged, " in ", exact.iterations, " iterations");
  out.writeln("by hand:        generation [1, 0.5], cost 23, incremental cost 20");
  const auto g = flat(exact.generation);
  out.writeln("incremental costs of the two generators (tight): ", 10.0 * g[0] + 10.0, " ", 16.0 * g[1] + 12.0);
  out.writeln("DC flows (p.u.): ", flat(exact.flow), "   largest balance error: ", std::abs(g[0] + g[1] - 1.5));
}
```

Output

```text
default solver: generation [0.99999744, 0.49999840], cost [22.99991664], converged true in 60 iterations
tight solver:   generation [1.00000000, 0.50000000], cost [23.00000000], converged true in 100 iterations
by hand:        generation [1, 0.5], cost 23, incremental cost 20
incremental costs of the two generators (tight): 20.00000000 20.00000000
DC flows (p.u.): [0.33333333, 0.83333333, 0.66666667]   largest balance error: 0.00000000
```
### Example 2

Post-outage flows after a secure dispatch.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <limits>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a square of four buses: generator A at bus 0 and generator B at bus 2, loads of 1 per unit at buses 1 and 3.
// Lines: 0 joins 0-1, 1 joins 1-2, 2 joins 2-3, 3 joins 3-0, each with x = 0.1.
static power::Net square() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, -1.0, 0.0, -1.0});
  net.buses.q = vec({0.0, -0.2, 0.0, -0.2});
  net.buses.vm = vec({1.0, 1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0, 0.0});
  net.gens.bus = {0, 2};
  net.gens.p = vec({1.0, 1.0});
  net.gens.q = vec({0.0, 0.0});
  net.lines.from = {0, 1, 2, 3};
  net.lines.to = {1, 2, 3, 0};
  net.lines.r = vec({0.01, 0.01, 0.01, 0.01});
  net.lines.x = vec({0.1, 0.1, 0.1, 0.1});
  return net;
}

// generator A: 0.25 g^2 + 10 g; generator B: 0.25 g^2 + 10.5 g; each between 0 and 3 per unit
static power::GenCost costs() {
  power::GenCost c;
  c.lower = vec({0.0, 0.0});
  c.upper = vec({3.0, 3.0});
  c.linear = vec({10.0, 10.5});
  c.quadratic = vec({0.25, 0.25});
  return c;
}

static power::DispatchOptions tight(bool secure) {
  power::DispatchOptions options;
  options.secure = secure;
  options.qp.abs_tol = 1e-10;
  options.qp.rel_tol = 1e-10;
  options.qp.max_iters = 50000;
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const power::Grid grid = power::make(square());
  const auto result = power::dispatch(grid, costs(), vec({1.3, 1.3, 1.3, 1.3}), tight(true));
  out.writeln("generation ", flat(result.generation), ", valid outages ", flat(result.outage_valid));
  const auto after = flat(result.post_flow);   // [outage line][monitored line]
  for (int k = 0; k < 4; ++k) out.writeln("flows after losing line ", k, ": ", after[4 * k], " ", after[4 * k + 1], " ", after[4 * k + 2], " ", after[4 * k + 3]);
  double worst = 0.0;
  for (int k = 0; k < 4; ++k)
    for (int m = 0; m < 4; ++m)
      if (m != k) worst = std::max(worst, std::abs(after[4 * k + m]));
  out.writeln("largest post-outage flow: ", worst, " (rating 1.3)");
  out.writeln("base flows ", flat(result.flow));
}
```

Output

```text
generation [1.3000, 0.7000], valid outages [1.0000, 1.0000, 1.0000, 1.0000]
flows after losing line 0: 0.0000 -1.0000 -0.3000 -1.3000
flows after losing line 1: 1.0000 0.0000 0.7000 -0.3000
flows after losing line 2: 0.3000 -0.7000 0.0000 -1.0000
flows after losing line 3: 1.3000 0.3000 1.0000 0.0000
largest post-outage flow: 1.3000 (rating 1.3)
base flows [0.6500, -0.3500, 0.3500, -0.6500]
```
### Example 3

An infeasible secure dispatch.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <limits>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a square of four buses: generator A at bus 0 and generator B at bus 2, loads of 1 per unit at buses 1 and 3.
// Lines: 0 joins 0-1, 1 joins 1-2, 2 joins 2-3, 3 joins 3-0, each with x = 0.1.
static power::Net square() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, -1.0, 0.0, -1.0});
  net.buses.q = vec({0.0, -0.2, 0.0, -0.2});
  net.buses.vm = vec({1.0, 1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0, 0.0});
  net.gens.bus = {0, 2};
  net.gens.p = vec({1.0, 1.0});
  net.gens.q = vec({0.0, 0.0});
  net.lines.from = {0, 1, 2, 3};
  net.lines.to = {1, 2, 3, 0};
  net.lines.r = vec({0.01, 0.01, 0.01, 0.01});
  net.lines.x = vec({0.1, 0.1, 0.1, 0.1});
  return net;
}

// generator A: 0.25 g^2 + 10 g; generator B: 0.25 g^2 + 10.5 g; each between 0 and 3 per unit
static power::GenCost costs() {
  power::GenCost c;
  c.lower = vec({0.0, 0.0});
  c.upper = vec({3.0, 3.0});
  c.linear = vec({10.0, 10.5});
  c.quadratic = vec({0.25, 0.25});
  return c;
}

static power::DispatchOptions tight(bool secure) {
  power::DispatchOptions options;
  options.secure = secure;
  options.qp.abs_tol = 1e-10;
  options.qp.rel_tol = 1e-10;
  options.qp.max_iters = 50000;
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const power::Grid grid = power::make(square());
  const auto limits = vec({0.9, 0.9, 0.9, 0.9});
  const auto plain = power::dispatch(grid, costs(), limits, tight(false));
  const auto secure = power::dispatch(grid, costs(), limits, tight(true));
  out.writeln("plain dispatch at rating 0.9:  converged ", plain.converged, ", generation ", flat(plain.generation));
  out.writeln("secure dispatch at rating 0.9: converged ", secure.converged, ", primal residual ", secure.primal_residual);
}
```

Output

```text
plain dispatch at rating 0.9:  converged true, generation [1.5000, 0.5000]
secure dispatch at rating 0.9: converged false, primal residual 0.1000
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="DispatchOptions.html">
    <span class="label">Previous</span>
    <span class="title">DispatchOptions</span>
  </a>
  <a class="next-link" href="dispatch.html">
    <span class="label">Next</span>
    <span class="title">dispatch</span>
  </a>
</div>
<!-- apiref:nav:end -->
