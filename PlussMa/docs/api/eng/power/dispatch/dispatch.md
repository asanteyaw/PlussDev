<!-- generated from dispatch.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::dispatch`

Least-cost DC dispatch of the generators, optionally secure against single-line outages.

## Signature

```cpp
power::DispatchResult power::dispatch(const power::Grid& grid, const power::GenCost& cost, const pluss::Series& line_limit, const power::DispatchOptions& options = {});
```

## What It Does

Minimises the total generation cost `sum(a g^2 + b g)` subject to the DC power balance, the generator bounds and the line ratings, as a quadratic program. The loads are the fixed bus injections of the grid; the generators listed in the grid are the decision variables. With `options.secure` the flow after every valid single-line outage must also stay within its rating, with the generation unchanged by the outage.

## Parameters

`grid`
: The compiled network; its generators are the decision variables and its bus injections the loads.

`cost`
: The bounds and cost curves of the generators.

`line_limit`
: The rating of each line, per unit, `[..., line]`, applied in both directions.

`options`
: Security and solver controls.

## Returns

A `DispatchResult`.

## Errors

Throws `std::invalid_argument` for a negative quadratic coefficient, a lower bound above its upper bound, or ratings and costs of the wrong length. An infeasible problem does not throw; it returns `converged` false.

## Interpretation and Use

The price of one more per-unit of demand is the incremental cost `lambda`; a binding rating separates the incremental costs of the generators. Dispatch is lossless with flat voltage: use `opf` where losses or voltages matter.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The optimum of a ring against its hand value.

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

A line rating that binds.

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
  out.print_defaults(PrintOptions{}.fixed(7).with_sep(""));
  const power::Grid grid = power::make(ring());
  const auto result = power::dispatch(grid, costs(), vec({10.0, 10.0, 0.6}), tight());
  out.writeln("generation: ", flat(result.generation), "   by hand 0.6, 0.9");
  out.writeln("flow on line 2: ", flat(result.flow)[2], "   rating 0.6");
  out.writeln("cost: ", flat(result.objective), "   by hand 25.08, against 23 without the rating");
  const auto g = flat(result.generation);
  out.writeln("incremental costs: ", 10.0 * g[0] + 10.0, " ", 16.0 * g[1] + 12.0, "   (no longer equal: the rating separates them)");
}
```

Output

```text
generation: [0.6000000, 0.9000000]   by hand 0.6, 0.9
flow on line 2: 0.6000000   rating 0.6
cost: [25.0800000]   by hand 25.08, against 23 without the rating
incremental costs: 16.0000000 26.4000000   (no longer equal: the rating separates them)
```
### Example 3

Many demand scenarios in one call.

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
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  power::Net batch = ring();
  batch.buses.p = pluss::series::make(std::vector<double>{0, 0, -1.0, 0, 0, -2.0}).reshape({2, 3});
  batch.buses.q = pluss::series::make(std::vector<double>{0, 0, -0.2, 0, 0, -0.4}).reshape({2, 3});
  const auto result = power::dispatch(power::make(batch), costs(), vec({10.0, 10.0, 10.0}), tight());
  const auto g = flat(result.generation);
  out.writeln("generation shape ", result.generation.sizes(), ": demand 1.0 -> ", g[0], " ", g[1], ";  demand 2.0 -> ", g[2], " ", g[3]);
  out.writeln("costs ", flat(result.objective));
  const double lambda = (440.0) / 26.0;
  out.writeln("by hand for demand 1.0: lambda ", lambda, ", generation ", (lambda - 10.0) / 10.0, " ", (lambda - 12.0) / 16.0);
}
```

Output

```text
generation shape [2, 2]: demand 1.0 -> 0.692308 0.307692;  demand 2.0 -> 1.307692 0.692308
costs [13.769231, 33.769231]
by hand for demand 1.0: lambda 16.923077, generation 0.692308 0.307692
```
### Example 4

Security against outages.

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
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const power::Grid grid = power::make(square());
  for (double rating : {2.0, 1.4, 1.3, 1.2}) {
    const auto limits = vec({rating, rating, rating, rating});
    const auto plain = power::dispatch(grid, costs(), limits, tight(false));
    const auto secure = power::dispatch(grid, costs(), limits, tight(true));
    const double a = rating, b = 2.0 - rating;
    out.writeln("rating ", rating, ": plain ", flat(plain.generation), " cost ", flat(plain.objective)[0], "   secure ", flat(secure.generation), " cost ", flat(secure.objective)[0], " (converged ", secure.converged, ")");
    if (rating < 1.5) out.writeln("          by hand: g_A = rating: ", a, " ", b, " cost ", 0.25 * a * a + 10 * a + 0.25 * b * b + 10.5 * b);
  }
}
```

Output

```text
rating 2.00000: plain [1.50000, 0.50000] cost 20.87500   secure [1.50000, 0.50000] cost 20.87500 (converged true)
rating 1.40000: plain [1.50000, 0.50000] cost 20.87500   secure [1.40000, 0.60000] cost 20.88000 (converged true)
          by hand: g_A = rating: 1.40000 0.60000 cost 20.88000
rating 1.30000: plain [1.50000, 0.50000] cost 20.87500   secure [1.30000, 0.70000] cost 20.89500 (converged true)
          by hand: g_A = rating: 1.30000 0.70000 cost 20.89500
rating 1.20000: plain [1.50000, 0.50000] cost 20.87500   secure [1.20000, 0.80000] cost 20.92000 (converged true)
          by hand: g_A = rating: 1.20000 0.80000 cost 20.92000
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="DispatchResult.html">
    <span class="label">Previous</span>
    <span class="title">DispatchResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
