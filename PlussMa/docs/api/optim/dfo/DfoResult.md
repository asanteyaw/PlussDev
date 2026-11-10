<!-- generated from DfoResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::DfoResult`

The outcome of a derivative-free run: the minimizer, the minimum, the status and which method ran.

`minimizer` is the best point found and `minimum` its objective value; `objective_vector` holds the objective values at the minimizer (one element for a scalar problem, several for a multi-objective black box, in which case `minimum` is the scalarised value). `status` says why the run stopped, `selected_solver` and `selection_reason` say which method ran and why, and `diagnostics` holds the counters. The mesh-adaptive methods also return the non-dominated points they found in `pareto_points` and `pareto_objectives` (one row per point); for every other method those are undefined.

## Fields

### `minimizer`

`pluss::Series`, default `undefined`.

The best point found.

### `objective_vector`

`pluss::Series`, default `undefined`.

Objective values at the minimizer.

### `pareto_points`

`pluss::Series`, default `undefined`.

Non-dominated points, one per row (mesh methods).

### `pareto_objectives`

`pluss::Series`, default `undefined`.

Their objective vectors, one per row.

### `minimum`

`double`, default `0`.

The objective value at the minimizer (scalarised for several objectives).

### `status`

`dfo::DfoStatus`, default `ReachedIterationBudget`.

Why the run stopped.

### `selected_solver`

`dfo::DfoSolverKind`, default `Auto`.

The method that ran.

### `selection_reason`

`std::string`, default `empty`.

Why that method was chosen.

### `diagnostics`

`dfo::DfoDiagnostics`, default `{}`.

Counters and last-state values.

## Guidance

Returned by `solve_dfo`, `DFO::solve`, `DFO::run` and `DfoBuilder::solve`.

## Practical Examples

### Example 1

Reading a result.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto f = [](const Series &x) { return (x - 3.0).square().sum(); };   // minimum 0 at (3, 3)

  const dfo::DfoResult r = dfo::solve_dfo(f, pluss::series::make({0.0, 0.0}));
  out.writeln("minimizer =", r.minimizer.items<double>());
  out.writeln("minimum =", r.minimum, ", objective_vector =", r.objective_vector.item<double>());
  out.writeln("status =", dfo::to_string(r.status), ", solver =", dfo::to_string(r.selected_solver));
}
```

Output

```text
minimizer = [3.000021, 3.000039]
minimum = 0.000000 , objective_vector = 0.000000
status = Converged , solver = NelderMead
```

### Example 2

The reason for the choice. A boxed problem is routed to differential evolution and the reason says so.

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

  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f).with_bounds(pluss::series::make({0.0, 0.0}), pluss::series::make({2.0, 2.0})).solve();
  out.writeln(dfo::to_string(r.selected_solver), "|", r.selection_reason);
  out.writeln("minimizer =", r.minimizer.items<double>());
}
```

Output

```text
DifferentialEvolution | boxed problem without explicit constraints; differential evolution is preferred
minimizer = [2.0000, 2.0000]
```

### Example 3

A Pareto set. Two objectives, `(x − 1)²` and `(x − 3)²`, cannot both be zero; a mesh-adaptive run returns the non-dominated points it found, one per row.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  dfo::MadsBlackboxAdvancedConfig mads;
  mads.pareto_archive_limit = 8;
  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({2.0}))
                               .with_blackbox([](const Series &x) {
                                 const Series a = (x - 1.0).square().sum(), b = (x - 3.0).square().sum();
                                 return a.reshape({1}).unite({b.reshape({1})}, 0);
                               })
                               .with_mads_config(mads)
                               .with_max_evaluations(300)
                               .solve();
  out.writeln("front size =", r.pareto_points.defined() ? r.pareto_points.size(0) : 0, ", objective vector =", r.objective_vector.items<double>());
}
```

Output

```text
front size = 8 , objective vector = [1.0000, 1.0000]
```

## See Also

- [`DfoStatus`](DfoStatus.md): the status
- [`DfoDiagnostics`](DfoDiagnostics.md): the counters

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="DfoProblem.html">
    <span class="label">Previous</span>
    <span class="title">DfoProblem</span>
  </a>
  <a class="next-link" href="DfoSolverKind.html">
    <span class="label">Next</span>
    <span class="title">DfoSolverKind</span>
  </a>
</div>
<!-- apiref:nav:end -->
