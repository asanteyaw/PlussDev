<!-- generated from ltmads_minimizer.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::ltmads_minimizer`

Minimises an objective with LT-MADS, mesh adaptive direct search with random lower-triangular poll directions.

## Signature

```cpp
dfo::BlackboxSolverOutcome dfo::ltmads_minimizer(const std::function<pluss::Series(const pluss::Series &)> &objective, const pluss::Series &initial, const dfo::MadsBlackboxConfig &config = {});
template <class Fn> dfo::BlackboxSolverOutcome dfo::ltmads_minimizer(Fn &&objective, const pluss::Series &initial, const dfo::MadsBlackboxConfig &config = {});
```

## What It Does

Mesh adaptive direct search evaluates the objective at trial points on a **mesh** around the best point. Each iteration may begin with a *search* step of `search_points` space-filling trial points, then *polls* `2n` directions at distance given by the **frame size**. A success moves to the better point and enlarges the mesh and frame (by `mesh_expand_factor` and `frame_expand_factor`); a failure refines them (by `mesh_refine_factor` and `frame_refine_factor`), the mesh faster than the frame, so the set of directions that can be reached becomes dense as the iteration converges. The run converges when the frame falls below `mesh_tolerance`.

LT-MADS builds its poll directions from a random lower-triangular integer matrix, the original MADS construction; it draws random numbers from `rng_seed`. It sets the poll scheme to `LtMadsRandom2N` whatever the configuration says. The objective may be non-smooth or discontinuous. Bounds in the configuration are respected by every trial point. For constraints, several objectives, failed evaluations, a cache or a surrogate use the black-box form ([`mads_blackbox_minimizer`](mads_blackbox_minimizer.md)). The result is a [`BlackboxSolverOutcome`](BlackboxSolverOutcome.md).

## Parameters

`Fn &&objective`
: A callable `Series (Series)` (real scalar) or `Series -> number`.

`const pluss::Series &initial`
: The starting point.

`const dfo::MadsBlackboxConfig &config`
: Mesh, frame, poll, search, budgets and bounds.

## Returns

A [`BlackboxSolverOutcome`](BlackboxSolverOutcome.md).

## Errors

- An objective that is not a real scalar throws.

## Practical Examples

### Example 1

The Rosenbrock function from the classic start. The default limit is 500 evaluations per variable, so this run ends at its evaluation limit, close to the minimum 0 at `(1, 1)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  const dfo::BlackboxSolverOutcome o = dfo::ltmads_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}));
  out.writeln("minimizer =", o.minimizer.items<double>(), ", minimum =", o.minimum);
  out.writeln("status =", dfo::to_string(o.status), ", evaluations =", o.diagnostics.evaluations);
}
```

Output

```text
minimizer = [0.93002, 0.86422] , minimum = 0.00495
status = ReachedEvaluationBudget , evaluations = 1000
```

### Example 2

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

  dfo::MadsBlackboxConfig config;
  config.lower_bounds = pluss::series::make({0.0, 0.0});
  config.upper_bounds = pluss::series::make({2.0, 2.0});
  const dfo::BlackboxSolverOutcome o = dfo::ltmads_minimizer(f, pluss::series::make({0.0, 0.0}), config);
  out.writeln("minimizer =", o.minimizer.items<double>(), ", minimum =", o.minimum);
}
```

Output

```text
minimizer = [2.0000, 2.0000] , minimum = 2.0000
```

### Example 3

A budget. An evaluation budget of 100 ends the run `ReachedEvaluationBudget`.

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

  dfo::MadsBlackboxConfig config;
  config.max_evaluations = 100;
  const dfo::BlackboxSolverOutcome o = dfo::ltmads_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config);
  out.writeln(dfo::to_string(o.status), ", evaluations =", o.diagnostics.evaluations, ", minimum =", o.minimum);
}
```

Output

```text
ReachedEvaluationBudget , evaluations = 100 , minimum = 3.3066
```

## See Also

- [`MadsBlackboxConfig`](MadsBlackboxConfig.md): the settings
- [`mads_blackbox_minimizer`](mads_blackbox_minimizer.md): the black-box form

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ltmads_blackbox_minimizer.html">
    <span class="label">Previous</span>
    <span class="title">ltmads_blackbox_minimizer</span>
  </a>
  <a class="next-link" href="mads_blackbox_minimizer.html">
    <span class="label">Next</span>
    <span class="title">mads_blackbox_minimizer</span>
  </a>
</div>
<!-- apiref:nav:end -->
