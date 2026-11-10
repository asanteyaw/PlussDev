<!-- generated from DfoStatus.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::DfoStatus`

Why a derivative-free run stopped.

`Converged` means the method's own stopping test was met: the simplex or the mesh became small, the population stopped improving, or the improvement fell below `relative_improvement_tolerance`. `ReachedIterationBudget` and `ReachedEvaluationBudget` mean `max_iterations` or `max_evaluations` ran out first; the minimizer is the best point found, which may still be a good answer. `ReachedTimeBudget` is reported by pattern search when its time limit passes. `InfeasibleStationary` is reported by pattern search when it stopped at a point that violates a constraint and could find no feasible direction.

A derivative-free method has no gradient to certify optimality, so `Converged` is a statement about the search, not a proof of a minimum. Check the minimum and, for a constrained problem, the constraint violation in the diagnostics.

## Fields

### `Converged`

`enumerator`, default `0`.

The method's stopping test was met.

### `ReachedIterationBudget`

`enumerator`, default `1`.

`max_iterations` ran out.

### `ReachedEvaluationBudget`

`enumerator`, default `2`.

`max_evaluations` ran out.

### `ReachedTimeBudget`

`enumerator`, default `3`.

The pattern-search time limit passed.

### `InfeasibleStationary`

`enumerator`, default `4`.

Pattern search stopped at an infeasible point with no feasible direction.

## Guidance

Read from `DfoResult::status`.

## Practical Examples

### Example 1

Converged and out of iterations. The same problem is solved with the default budget and with three iterations.

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

  const Series start = pluss::series::make({0.0, 0.0});
  const dfo::DfoResult full = dfo::solve_dfo(f, start);
  const dfo::DfoResult short_run = dfo::DFO::build(start).with_objective(f).with_max_iterations(3).solve();
  out.writeln("default :", dfo::to_string(full.status), ", iterations =", full.diagnostics.iterations);
  out.writeln("3 iters :", dfo::to_string(short_run.status), ", iterations =", short_run.diagnostics.iterations);
}
```

Output

```text
default : Converged , iterations = 49
3 iters : ReachedIterationBudget , iterations = 3
```

### Example 2

Out of evaluations. A budget of 10 evaluations ends the run with `ReachedEvaluationBudget`; the method checks the budget between steps, so the count can be one over.

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

  const dfo::DfoResult result = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f).with_max_evaluations(10).solve();
  out.writeln(dfo::to_string(result.status), ", evaluations =", result.diagnostics.evaluations);
}
```

Output

```text
ReachedEvaluationBudget , evaluations = 11
```

### Example 3

Using the status. A helper that accepts a result only when the search converged.

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

  const auto usable = [](const dfo::DfoResult &r) { return r.status == dfo::DfoStatus::Converged; };
  const Series start = pluss::series::make({0.0, 0.0});
  out.writeln("default budget usable :", usable(dfo::solve_dfo(f, start)));
  out.writeln("5 evaluations usable  :", usable(dfo::DFO::build(start).with_objective(f).with_max_evaluations(5).solve()));
}
```

Output

```text
default budget usable : true
5 evaluations usable  : false
```

## See Also

- [`DfoResult`](DfoResult.md): holds it
- [`to_string`](to_string.md): names a status

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="DfoSolverKind.html">
    <span class="label">Previous</span>
    <span class="title">DfoSolverKind</span>
  </a>
  <a class="next-link" href="DifferentialEvolutionConfig.html">
    <span class="label">Next</span>
    <span class="title">DifferentialEvolutionConfig</span>
  </a>
</div>
<!-- apiref:nav:end -->
