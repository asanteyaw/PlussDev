<!-- generated from pattern_search_minimizer.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::pattern_search_minimizer`

Minimises an objective by polling a pattern of directions on an adaptive mesh, with bounds, linear rows and nonlinear constraints.

## Signature

```cpp
dfo::PSOutcome dfo::pattern_search_minimizer(const std::function<pluss::Series(const pluss::Series &)> &objective, const pluss::Series &initial, const dfo::PSConfig &config = {});
template <class Fn> dfo::PSOutcome dfo::pattern_search_minimizer(Fn &&objective, const pluss::Series &initial, const dfo::PSConfig &config = {});
```

## What It Does

Pattern search works on a **mesh**. At each iteration it first tries a *search* step (extra points chosen to jump ahead, if enabled), then *polls*: it evaluates the points `x ± mesh · d` along a set of directions `d` around the best point. A poll that finds a better point moves there and **expands** the mesh by `mesh_expansion`; a poll that fails **contracts** the mesh by `mesh_contraction`. The run converges when the mesh falls below `mesh_tolerance`. Because it only compares values, the objective may be non-smooth.

Constraints are what set pattern search apart from the simplex: bounds (`lower_bounds`, `upper_bounds`), linear inequalities `A x ≤ b` and equalities `Aeq x = beq` are enforced exactly on every trial point, and nonlinear inequality and equality constraints are handled by a barrier, a merit penalty or an augmented Lagrangian ([`PSConstraintPolicy`](PSConstraintPolicy.md)). This is the method [`Auto`](DfoSolverKind.md) uses for constrained problems.

The poll directions are a positive basis, by default a random orthonormal one refreshed every iteration, so that on a diagonal or curved boundary some direction can always follow it. Repeated points are answered from a cache.

## Parameters

`Fn &&objective`
: A callable `Series (Series)` (real scalar) or `Series -> number`.

`const pluss::Series &initial`
: The starting point (should be feasible for the linear rows and bounds).

`const dfo::PSConfig &config`
: Mesh, poll, search, constraints, budgets and bounds.

## Returns

A [`PSOutcome`](PSOutcome.md).

## Errors

- An objective that is not a real scalar throws.

## Practical Examples

### Example 1

The Rosenbrock function with every default. The minimum is 0 at `(1, 1)`. The default limit is 100 iterations per variable, so this run ends at its iteration limit, `ReachedIterationBudget`, close to the minimum; raise `max_iterations` to let the mesh converge.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  const dfo::PSOutcome o = dfo::pattern_search_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}));
  out.writeln("minimizer =", o.minimizer.items<double>(), ", minimum =", o.minimum);
  out.writeln("status =", dfo::to_string(o.status), ", evaluations =", o.diagnostics.evaluations, ", final mesh =", o.diagnostics.mesh_size);
}
```

Output

```text
minimizer = [0.992966, 0.986126] , minimum = 0.000052
status = ReachedIterationBudget , evaluations = 663 , final mesh = 0.000977
```

### Example 2

A nonlinear constraint. Minimise `Σ (xᵢ − 2)²` inside the unit disc; the exact optimum is `(0.7071, 0.7071)` with value 3.3431.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x - 2.0).square().sum(); };   // exact constrained optimum 3.3431 at (0.7071, 0.7071)
  dfo::PSConfig config;
  config.nonlinear_constraints = [](const Series &x) {
    dfo::PSConstraintValues v;
    v.inequality = (x.square().sum() - 1.0).reshape({1});   // inside the unit disc
    return v;
  };

  const dfo::PSOutcome o = dfo::pattern_search_minimizer(f, pluss::series::make({0.0, 0.0}), config);
  out.writeln("minimizer =", o.minimizer.items<double>(), ", minimum =", o.minimum, ", violation =", o.diagnostics.max_constraint_violation);
}
```

Output

```text
minimizer = [0.7059, 0.7083] , minimum = 3.3432 , violation = 0.0000
```

### Example 3

Linear rows and bounds, enforced exactly. Minimise `Σ (xᵢ − 2)²` with `x₀ + x₁ + x₂ ≤ 3` and `x₀ − x₁ = 0.5` in `[0, 5]³`; by hand the optimum is `(1.25, 0.75, 1)` with value 3.125.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x - 2.0).square().sum(); };
  dfo::PSConfig config;
  config.lower_bounds = pluss::series::make({0.0, 0.0, 0.0});
  config.upper_bounds = pluss::series::make({5.0, 5.0, 5.0});
  config.linear_ineq_matrix = pluss::series::make({1.0, 1.0, 1.0}).reshape({1, 3});
  config.linear_ineq_rhs = pluss::series::make({3.0});
  config.linear_eq_matrix = pluss::series::make({1.0, -1.0, 0.0}).reshape({1, 3});
  config.linear_eq_rhs = pluss::series::make({0.5});
  const dfo::PSOutcome o = dfo::pattern_search_minimizer(f, pluss::series::make({0.5, 0.0, 0.5}), config);
  out.writeln("minimizer =", o.minimizer.items<double>(), ", minimum =", o.minimum);
}
```

Output

```text
minimizer = [1.2500, 0.7500, 1.0000] , minimum = 3.1250
```

### Example 4

A discontinuous objective. A staircase has no gradient anywhere; the poll steps down it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x * 2.0).floor().abs().sum(); };
  const dfo::PSOutcome o = dfo::pattern_search_minimizer(f, pluss::series::make({3.2, -2.7}));
  out.writeln("minimum =", o.minimum, ", status =", dfo::to_string(o.status));
}
```

Output

```text
minimum = 0.0000 , status = Converged
```

## See Also

- [`PSConfig`](PSConfig.md): the settings
- [`DfoSolverKind`](DfoSolverKind.md): the front door's `PatternSearch`

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="particle_swarm_minimizer.html">
    <span class="label">Previous</span>
    <span class="title">particle_swarm_minimizer</span>
  </a>
  <a class="next-link" href="psdmads_blackbox_minimizer.html">
    <span class="label">Next</span>
    <span class="title">psdmads_blackbox_minimizer</span>
  </a>
</div>
<!-- apiref:nav:end -->
