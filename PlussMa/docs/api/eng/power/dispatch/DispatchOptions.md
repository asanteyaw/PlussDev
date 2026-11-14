<!-- generated from DispatchOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::DispatchOptions`

Security switch and quadratic-programming controls of `dispatch`.

## Signature

```cpp
power::DispatchOptions
```

## What It Does

Controls for `dispatch`: whether line outages must be survived, how the outage factors decide validity, and the options of the quadratic-programming solver.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `secure` | Require every post-outage flow to stay within the ratings, with the same generation before and after the outage. | false |
| `outage_tolerance` | The validity threshold of `lodf` for the outages. | 1e-10 |
| `qp` | The options of the quadratic-programming solver: `abs_tol`, `rel_tol`, `max_iters` and others. | solver defaults |

## Returns

A value; validated by `dispatch`.

## Errors

A non-positive or non-finite `outage_tolerance` is rejected.

## Interpretation and Use

The default solver tolerance is about `1e-5`, which is generous for planning. Tighten `qp.abs_tol` and `qp.rel_tol` and raise `qp.max_iters` to compare with an exact value.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Default and tight solver tolerances.

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

Security on and off for four ratings.

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
### Example 3

An outage tolerance that invalidates every outage.

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
  power::Net spur = square();
  spur.buses.kind.push_back(power::BusKind::PQ);
  spur.buses.p = vec({0.0, -1.0, 0.0, -1.0, -0.2});
  spur.buses.q = vec({0.0, -0.2, 0.0, -0.2, -0.04});
  spur.buses.vm = vec({1.0, 1.0, 1.0, 1.0, 1.0});
  spur.buses.va = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  spur.lines.from.push_back(1);
  spur.lines.to.push_back(4);
  spur.lines.r = vec({0.01, 0.01, 0.01, 0.01, 0.01});
  spur.lines.x = vec({0.1, 0.1, 0.1, 0.1, 0.1});
  const power::Grid grid = power::make(spur);
  const auto result = power::dispatch(grid, costs(), vec({1.6, 1.6, 1.6, 1.6, 1.6}), tight(true));
  out.writeln("generation ", flat(result.generation), ", converged ", result.converged);
  out.writeln("valid outages (one per line) ", flat(result.outage_valid));
  const auto after = flat(result.post_flow);
  out.writeln("flows after losing the spur line: ", after[4 * 5], " ", after[4 * 5 + 1], " ", after[4 * 5 + 2], " ", after[4 * 5 + 3], " ", after[4 * 5 + 4]);

  // the outage tolerance: a large one declares every outage invalid, and the problem reduces to the plain dispatch
  power::DispatchOptions strict = tight(true);
  strict.outage_tolerance = 0.9;
  const auto none = power::dispatch(grid, costs(), vec({1.6, 1.6, 1.6, 1.6, 1.6}), strict);
  out.writeln("with outage_tolerance 0.9, valid outages: ", flat(none.outage_valid), ", generation ", flat(none.generation));
}
```

Output

```text
generation [1.6000, 0.6000], converged true
valid outages (one per line) [1.0000, 1.0000, 1.0000, 1.0000, 0.0000]
flows after losing the spur line: nan nan nan nan nan
with outage_tolerance 0.9, valid outages: [0.0000, 0.0000, 0.0000, 0.0000, 0.0000], generation [1.6000, 0.6000]
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="GenCost.html">
    <span class="label">Previous</span>
    <span class="title">GenCost</span>
  </a>
  <a class="next-link" href="DispatchResult.html">
    <span class="label">Next</span>
    <span class="title">DispatchResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
