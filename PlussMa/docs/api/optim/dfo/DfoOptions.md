<!-- generated from DfoOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::DfoOptions`

Method choice, budgets, tolerance, seed and the per-method settings of a derivative-free run.

The common settings are applied to whichever method runs: a positive `max_iterations` or `max_evaluations` overrides that method's own default, `relative_improvement_tolerance` sets its stopping tolerance when positive, and the seed, the double-precision promotion and the iteration display are passed on. Left at zero, the budgets and the tolerance keep each method's own default. The per-method members (`nelder_mead`, `differential_evolution`, `particle_swarm`, `pattern_search`, `mads`, and so on) carry every other setting of that method and are read only when that method runs; the front door sets bounds and the poll scheme itself, so the bounds fields of those members are overwritten from the problem.

## Fields

### `solver`

`dfo::DfoSolverKind`, default `Auto`.

The method to run, or `Auto`.

### `max_iterations`

`std::int64_t`, default `0`.

When positive, the iteration (generation) budget of the method that runs; zero keeps the method's own limit (a multiple of the number of variables for Nelder-Mead, pattern search and MADS).

### `max_evaluations`

`std::int64_t`, default `0`.

When positive, the objective-evaluation budget of the method that runs.

### `max_total_evaluations`

`std::int64_t`, default `0`.

MADS only: a budget that also counts cache hits.

### `check_every`

`std::int64_t`, default `10`.

How often (in iterations) the improvement test is checked, for the population and MADS methods.

### `relative_improvement_tolerance`

`double`, default `0`.

When positive, replaces the method's own stopping tolerance.

### `rng_seed`

`std::uint64_t`, default `5489`.

Seed of the random numbers of the evolutionary, swarm and randomised-poll methods.

### `promote_cpu_to_float64`

`bool`, default `true`.

Whether single-precision CPU inputs are computed in double precision.

### `display_iter`

`bool`, default `false`.

Print a line per iteration.

### `auto_policy`

`dfo::DfoAutoPolicy`, default `{}`.

Thresholds for `Auto`.

### `nelder_mead`

`dfo::NelderMeadConfig`, default `{}`.

Settings of Nelder-Mead.

### `differential_evolution`

`dfo::DifferentialEvolutionConfig`, default `{}`.

Settings of differential evolution.

### `differential_evolution_prmm`

`dfo::DifferentialEvolutionPrmmConfig`, default `{}`.

Settings of the adaptive variant.

### `particle_swarm`

`dfo::ParticleSwarmConfig`, default `{}`.

Settings of the swarm.

### `particle_swarm_dv`

`dfo::ParticleSwarmDvConfig`, default `{}`.

Settings of the differential-velocity swarm.

### `pattern_search`

`dfo::PSConfig`, default `{}`.

Settings of pattern search.

### `mads`

`dfo::MadsBlackboxAdvancedConfig`, default `{}`.

Settings of the MADS kinds.

## Guidance

Passed to `DFO`, `DFO::solve`, `solve_dfo` or `DfoBuilder::with_options`.

## Practical Examples

### Example 1

Budgets. The same Rosenbrock problem with a small and a generous evaluation budget; the status shows which ran out.

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

  for (const std::int64_t budget : {60, 2000}) {
    dfo::DfoOptions options;
    options.solver = dfo::DfoSolverKind::NelderMead;
    options.max_evaluations = budget;
    const dfo::DfoResult r = dfo::solve_dfo(rosenbrock, pluss::series::make({-1.2, 1.0}), options);
    out.writeln("budget", budget, ":", dfo::to_string(r.status), ", evaluations =", r.diagnostics.evaluations, ", minimum =", r.minimum);
  }
}
```

Output

```text
budget 60 : ReachedEvaluationBudget , evaluations = 61 , minimum = 0.669165
budget 2000 : Converged , evaluations = 159 , minimum = 0.000000
```

### Example 2

Seeds. The evolutionary methods draw random numbers; the same seed repeats a run exactly and a different seed gives a different run.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto f = [](const Series &x) { return (x - pluss::series::make({1.0, -2.0, 0.5, 3.0})).square().sum(); };   // minimum 0 at (1, -2, 0.5, 3)

  const Series start = pluss::series::make({0.0, 0.0, 0.0, 0.0});
  for (const std::uint64_t seed : {1, 1, 2}) {
    dfo::DfoOptions options;
    options.solver = dfo::DfoSolverKind::DifferentialEvolution;
    options.rng_seed = seed;
    options.max_iterations = 40;
    options.differential_evolution.population_size = 40;
    const dfo::DfoResult r = dfo::solve_dfo(f, start, options);
    out.writeln("seed", seed, ": minimum =", r.minimum, ", evaluations =", r.diagnostics.evaluations);
  }
}
```

Output

```text
seed 1 : minimum = 0.01077316 , evaluations = 1640
seed 1 : minimum = 0.01077316 , evaluations = 1640
seed 2 : minimum = 0.00559327 , evaluations = 1640
```

### Example 3

Per-method settings. The Nelder-Mead member changes the simplex; here a tighter value tolerance asks for more accuracy.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(12));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  for (const double tol : {1e-4, 1e-10}) {
    dfo::DfoOptions options;
    options.solver = dfo::DfoSolverKind::NelderMead;
    options.nelder_mead.value_tolerance = tol;
    options.nelder_mead.position_tolerance = tol;
    const dfo::DfoResult r = dfo::solve_dfo(rosenbrock, pluss::series::make({-1.2, 1.0}), options);
    out.writeln("tolerance", tol, ": evaluations =", r.diagnostics.evaluations, ", minimum =", r.minimum);
  }
}
```

Output

```text
tolerance 0.000100000000 : evaluations = 159 , minimum = 0.000000000818
tolerance 0.000000000100 : evaluations = 249 , minimum = 0.000000000000
```

### Example 4

Showing progress. `display_iter` makes the method print a line per iteration.

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

  dfo::DfoOptions options;
  options.solver = dfo::DfoSolverKind::NelderMead;
  options.max_iterations = 4;
  options.display_iter = true;
  const dfo::DfoResult r = dfo::solve_dfo(f, pluss::series::make({0.0, 0.0}), options);
  out.writeln("done:", dfo::to_string(r.status));
}
```

Output

```text
iter=1 evals=3 f=17.9985 phase=seed
iter=2 evals=5 f=17.9955 phase=expand
iter=3 evals=6 f=17.9955 phase=reflect
iter=4 evals=8 f=17.9895 phase=expand
done: ReachedIterationBudget
```

## See Also

- [`DfoAutoPolicy`](DfoAutoPolicy.md): the `Auto` thresholds
- [`DfoSolverKind`](DfoSolverKind.md): the methods

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="DfoDiagnostics.html">
    <span class="label">Previous</span>
    <span class="title">DfoDiagnostics</span>
  </a>
  <a class="next-link" href="DfoProblem.html">
    <span class="label">Next</span>
    <span class="title">DfoProblem</span>
  </a>
</div>
<!-- apiref:nav:end -->
