<!-- generated from PSConfig.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::PSConfig`

Mesh, poll, search, constraint, cache, bound and budget settings of pattern search.

The settings fall into groups. **Stopping:** `mesh_tolerance`, `step_tolerance` and `value_tolerance` for convergence, `max_iterations`, `max_evaluations` and `time_limit_seconds` for budgets (zero keeps the default limits of 100 iterations and 2000 evaluations per variable; the time limit is infinite by default), and `check_every` with `relative_improvement_tolerance` and `min_improvement` for the improvement test. **Mesh:** `initial_mesh_size`, `max_mesh_size`, `mesh_expansion`, `mesh_contraction` and the per-variable scaling `mesh_scaling` with the `nups_*` settings. **Poll:** `poll_pattern`, `polling_order`, `complete_poll`, `direction_family`, `basis_family` and `basis_refresh_interval`. **Search:** `search_strategy` with `search_expansion`, `search_trials` and `search_complete`. **Constraints:** bounds, the linear rows, `nonlinear_constraints` and the policy settings `constraint_policy`, `initial_penalty`, `penalty_growth`, `al_*`. **Cache:** `enable_cache`, `cache_max_entries`, `cache_tolerance`. Also the seed, the double-precision promotion and the display.

Every group has its own example on the field pages of the enumerations it uses; the examples below change one group at a time.

## Fields

### `mesh_tolerance`

`double`, default `1e-6`.

Converged when the mesh size falls below this.

### `step_tolerance`

`double`, default `1e-6`.

Converged when the accepted step is below this.

### `value_tolerance`

`double`, default `1e-6`.

Converged when the improvement is below this.

### `constraint_tolerance`

`double`, default `1e-6`.

A constraint violation up to this is accepted as feasible.

### `active_constraint_tolerance`

`double`, default `1e-3`.

A constraint within this of its limit counts as active when building feasible directions.

### `max_iterations`

`std::int64_t`, default `0`.

Iteration limit; 0 uses 100 × the number of variables.

### `max_evaluations`

`std::int64_t`, default `0`.

Evaluation limit; 0 uses 2000 × the number of variables.

### `time_limit_seconds`

`double`, default `+inf`.

Wall-clock limit; the run ends `ReachedTimeBudget`.

### `initial_mesh_size`

`double`, default `1`.

Starting mesh size.

### `max_mesh_size`

`double`, default `+inf`.

Largest mesh size after expansion.

### `mesh_expansion`

`double`, default `2`.

Mesh multiplier after a successful poll.

### `mesh_contraction`

`double`, default `0.5`.

Mesh multiplier after a failed poll.

### `check_every`

`std::int64_t`, default `0`.

Iterations between improvement tests; 0 disables the test.

### `relative_improvement_tolerance`

`double`, default `0`.

Stop when the relative improvement over `check_every` iterations is below this.

### `min_improvement`

`double`, default `0`.

A poll point must improve by more than this to be accepted.

### `poll_pattern`

`dfo::PSPollPattern`, default `PositiveBasis2N`.

Which positive basis is polled.

### `polling_order`

`dfo::PSPollingOrder`, default `Consecutive`.

The order the poll points are tried.

### `complete_poll`

`bool`, default `false`.

Evaluate every poll point even after one improves.

### `direction_family`

`dfo::PSDirectionFamily`, default `GSSAugmented`.

How directions are adapted to active constraints.

### `basis_family`

`dfo::PSBasisFamily`, default `RandomOrtho`.

Coordinate axes or a random orthonormal basis.

### `basis_refresh_interval`

`std::int64_t`, default `1`.

Iterations between new random bases.

### `mesh_scaling`

`dfo::PSMeshScaling`, default `RangeLog`.

Per-variable mesh scaling.

### `nups_expansion`

`double`, default `1.6`.

NUPS scale growth on success.

### `nups_contraction`

`double`, default `0.7`.

NUPS scale shrink on failure.

### `nups_decay`

`double`, default `0.98`.

NUPS decay per iteration.

### `nups_min_scale`

`double`, default `1e-3`.

Smallest NUPS scale.

### `nups_max_scale`

`double`, default `1e6`.

Largest NUPS scale.

### `search_strategy`

`dfo::PSSearchStrategy`, default `ExpandAlongSuccess`.

The search step before each poll.

### `search_expansion`

`double`, default `2`.

How far the last successful step is extended.

### `search_trials`

`std::int64_t`, default `12`.

Random trial points per search.

### `search_complete`

`bool`, default `false`.

Evaluate every search point even after one improves.

### `constraint_policy`

`dfo::PSConstraintPolicy`, default `AugmentedLagrangian`.

How nonlinear constraint violations are handled.

### `initial_penalty`

`double`, default `10`.

Starting penalty weight.

### `penalty_growth`

`double`, default `10`.

Penalty multiplier between rounds (merit penalty).

### `al_max_outer_iterations`

`std::int64_t`, default `20`.

Most augmented-Lagrangian rounds.

### `al_penalty_growth`

`double`, default `5`.

Penalty growth when a round does not reduce the violation enough.

### `al_violation_reduction_ratio`

`double`, default `0.5`.

Required violation reduction per round to keep the penalty.

### `al_multiplier_clip`

`double`, default `1e8`.

Largest multiplier estimate.

### `enable_cache`

`bool`, default `true`.

Answer repeated points from a cache.

### `cache_max_entries`

`std::int64_t`, default `10000`.

Cache capacity.

### `cache_tolerance`

`double`, default `1e-12`.

Two points closer than this count as the same.

### `lower_bounds`

`pluss::Series`, default `undefined`.

Lower bound per variable.

### `upper_bounds`

`pluss::Series`, default `undefined`.

Upper bound per variable.

### `linear_ineq_matrix`

`pluss::Series`, default `undefined`.

Matrix `A` of `A x ≤ b`, one column per variable.

### `linear_ineq_rhs`

`pluss::Series`, default `undefined`.

Right side `b`.

### `linear_eq_matrix`

`pluss::Series`, default `undefined`.

Matrix `Aeq` of `Aeq x = beq`.

### `linear_eq_rhs`

`pluss::Series`, default `undefined`.

Right side `beq`.

### `nonlinear_constraints`

`std::function<dfo::PSConstraintValues(const pluss::Series &)>`, default `empty`.

Nonlinear inequality and equality values.

### `rng_seed`

`std::uint64_t`, default `5489`.

Seed of the random basis, random order and random search.

### `promote_cpu_to_float64`

`bool`, default `true`.

Compute single-precision CPU inputs in double precision.

### `display`

`dfo::PSDisplay`, default `None`.

Print a line per iteration when `Iter`.

## Guidance

Passed to `pattern_search_minimizer`, or set as `DfoOptions::pattern_search`.

## Practical Examples

### Example 1

The mesh. A start that is far too small takes many expansions; a larger starting mesh reaches the valley sooner.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  for (const double mesh : {0.01, 1.0, 8.0}) {
    dfo::PSConfig config;
    config.initial_mesh_size = mesh;
    const dfo::PSOutcome o = dfo::pattern_search_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config);
    out.writeln("initial mesh", mesh, ": evaluations =", o.diagnostics.evaluations, ", minimum =", o.minimum);
  }
}
```

Output

```text
initial mesh 0.01000000 : evaluations = 668 , minimum = 0.00000003
initial mesh 1.00000000 : evaluations = 663 , minimum = 0.00005153
initial mesh 8.00000000 : evaluations = 683 , minimum = 0.00007765
```

### Example 2

Budgets. An evaluation budget ends the run with `ReachedEvaluationBudget`; an iteration budget with `ReachedIterationBudget`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  dfo::PSConfig by_evaluations, by_iterations;
  by_evaluations.max_evaluations = 80;
  by_iterations.max_iterations = 10;
  out.writeln("80 evaluations :", dfo::to_string(dfo::pattern_search_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), by_evaluations).status));
  out.writeln("10 iterations  :", dfo::to_string(dfo::pattern_search_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), by_iterations).status));
}
```

Output

```text
80 evaluations : ReachedEvaluationBudget
10 iterations  : ReachedIterationBudget
```

### Example 3

Tolerances. A looser `mesh_tolerance` stops the mesh refinement sooner. The iteration limit is raised so that each run can converge.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  for (const double tol : {1e-3, 1e-6, 1e-9}) {
    dfo::PSConfig config;
    config.mesh_tolerance = tol;
    config.max_iterations = 5000;
    const dfo::PSOutcome o = dfo::pattern_search_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config);
    out.writeln("mesh tolerance", tol, ": evaluations =", o.diagnostics.evaluations, ", minimum =", o.minimum);
  }
}
```

Output

```text
mesh tolerance 0.0010000000 : evaluations = 242 , minimum = 0.0036804591
mesh tolerance 0.0000010000 : evaluations = 950 , minimum = 0.0000000000
mesh tolerance 0.0000000010 : evaluations = 950 , minimum = 0.0000000000
```

### Example 4

Bounds. The unconstrained minimum `(3, 3)` is outside `[0, 2]²`; the box holds the answer at the corner.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x - 3.0).square().sum(); };   // minimum 0 at (3, 3)

  dfo::PSConfig config;
  config.lower_bounds = pluss::series::make({0.0, 0.0});
  config.upper_bounds = pluss::series::make({2.0, 2.0});
  const dfo::PSOutcome o = dfo::pattern_search_minimizer(f, pluss::series::make({0.0, 0.0}), config);
  out.writeln("minimizer =", o.minimizer.items<double>());
}
```

Output

```text
minimizer = [2.0000, 2.0000]
```

## See Also

- [`pattern_search_minimizer`](pattern_search_minimizer.md): uses it
- [`DfoOptions`](DfoOptions.md): holds one

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="PSBasisFamily.html">
    <span class="label">Previous</span>
    <span class="title">PSBasisFamily</span>
  </a>
  <a class="next-link" href="PSConstraintPolicy.html">
    <span class="label">Next</span>
    <span class="title">PSConstraintPolicy</span>
  </a>
</div>
<!-- apiref:nav:end -->
