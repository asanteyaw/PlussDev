<!-- generated from GenCost.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::GenCost`

Cost curves and output bounds of the generators.

## Signature

```cpp
power::GenCost
```

## What It Does

Describes, for every generator of the grid, the bounds on its active output and its cost as a polynomial of the output. The same type serves `dispatch` (active terms only) and `opf` (also the reactive terms).

## Fields

| Field | Meaning | Default |
|---|---|---|
| `lower` | Active-power lower bound of each generator, per unit, `[..., generator]`. | required |
| `upper` | Active-power upper bound, `[..., generator]`; may be infinite. | required |
| `linear` | Coefficient of `p`, cost per per-unit, `[..., generator]`. | required |
| `quadratic` | Coefficient of `p^2`, `[..., generator]`; must be non-negative for `dispatch`. | required |
| `constant` | Constant cost, which does not change the optimum. | undefined (zero) |
| `q_linear` | Coefficient of the reactive output `q`, used by `opf` only. | undefined (zero) |
| `q_quadratic` | Coefficient of `q^2`, used by `opf` only. | undefined (zero) |
| `q_constant` | Constant reactive cost, used by `opf` only. | undefined (zero) |

## Returns

A value; validated by `dispatch` and `opf`.

## Errors

A lower bound above its upper bound, a negative quadratic coefficient for `dispatch`, or a length that is not the number of generators is rejected.

## Interpretation and Use

The optimum of `dispatch` is where every generator that is not at a bound has the same incremental cost `2 a g + b`. The leading axes give one cost curve per scenario. A fixed cost does not change how output is shared.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A quadratic cost solved against its hand optimum.

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

Upper bounds, an infinite bound and fixed costs.

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
  const auto ratings = vec({10.0, 10.0, 10.0});

  power::GenCost capped = costs();
  capped.upper = vec({0.8, 2.0});
  out.writeln("g0 limited to 0.8:        ", flat(power::dispatch(power::make(ring()), capped, ratings, tight()).generation));

  power::Net one_off = ring();
  one_off.gens.active = {true, false};
  out.writeln("generator 1 out of service: ", flat(power::dispatch(power::make(one_off), costs(), ratings, tight()).generation));

  power::GenCost unbounded = costs();
  unbounded.upper = vec({std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()});
  out.writeln("no upper bounds:          ", flat(power::dispatch(power::make(ring()), unbounded, ratings, tight()).generation));

  // a constant cost does not change the decision; the dispatch objective holds the quadratic and linear terms only
  power::GenCost with_constant = costs();
  with_constant.constant = vec({100.0, 50.0});
  const auto k = power::dispatch(power::make(ring()), with_constant, ratings, tight());
  out.writeln("with fixed costs 100 and 50: generation ", flat(k.generation), ", objective ", flat(k.objective));
}
```

Output

```text
g0 limited to 0.8:        [0.800000, 0.700000]
generator 1 out of service: [1.500000, 0.000000]
no upper bounds:          [1.000000, 0.500000]
with fixed costs 100 and 50: generation [1.000000, 0.500000], objective [23.000000]
```
### Example 3

A concave cost refused.

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
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  const power::Grid grid = power::make(ring());
  const auto ratings = vec({10.0, 10.0, 10.0});

  // 1. the default accuracy is about five digits
  const auto rough = power::dispatch(grid, costs(), ratings);
  out.writeln("default solver: |g0 - 1| = ", std::abs(flat(rough.generation)[0] - 1.0), ", equal to 1 exactly: ", flat(rough.generation)[0] == 1.0);

  // 2. more demand than the generators can supply: the load is 1.5 and each unit is capped at 0.5
  power::GenCost small = costs();
  small.upper = vec({0.5, 0.5});
  const auto infeasible = power::dispatch(grid, small, ratings);
  out.writeln("demand 1.5, capacity 1.0: converged ", infeasible.converged, ", primal residual ", infeasible.primal_residual);

  // 3. a concave cost curve
  power::GenCost concave = costs();
  concave.quadratic = vec({-1.0, 8.0});
  try {
    (void)power::dispatch(grid, concave, ratings);
  } catch (const std::exception &e) {
    std::cout << "negative quadratic coefficient: " << e.what() << "\n";
  }

  // 4. ratings for the wrong number of lines
  try {
    (void)power::dispatch(grid, costs(), vec({10.0, 10.0}));
  } catch (const std::exception &e) {
    std::cout << "two ratings for three lines: " << e.what() << "\n";
  }
}
```

Output

```text
default solver: |g0 - 1| = 2.56e-06, equal to 1 exactly: false
demand 1.5, capacity 1.0: converged false, primal residual 1.67e-01
negative quadratic coefficient: eng::power::dispatch: quadratic costs must be non-negative
two ratings for three lines: eng::power::dispatch: line limits must be real floating [..., elements]
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="DispatchOptions.html">
    <span class="label">Next</span>
    <span class="title">DispatchOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
