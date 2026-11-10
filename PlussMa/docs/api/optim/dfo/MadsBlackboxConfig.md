<!-- generated from MadsBlackboxConfig.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::MadsBlackboxConfig`

Mesh, frame, poll, search, budget and bound settings of the mesh-adaptive methods.

The **frame size** is the distance of the poll points; the **mesh size** is the grid the trial points lie on and is never larger than the frame. After a failed iteration the frame and mesh are multiplied by `frame_refine_factor` and `mesh_refine_factor`; after a success by `frame_expand_factor` and `mesh_expand_factor`. The run converges when the frame is at most `mesh_tolerance`. `opportunistic` stops each poll at its first improvement, which saves evaluations; turning it off evaluates the whole poll. `search_points` adds a space-filling search before each poll. The advanced configuration ([`MadsBlackboxAdvancedConfig`](MadsBlackboxAdvancedConfig.md)) is built on this one.

## Fields

### `max_iterations`

`std::int64_t`, default `0`.

Iteration limit; 0 uses 250 × the number of variables.

### `max_evaluations`

`std::int64_t`, default `0`.

Evaluation limit; 0 uses 500 × the number of variables.

### `check_every`

`std::int64_t`, default `10`.

Iterations between improvement tests.

### `search_points`

`std::int64_t`, default `0`.

Space-filling trial points evaluated before each poll; 0 for no search.

### `relative_improvement_tolerance`

`double`, default `1e-8`.

Stop when the relative improvement over `check_every` iterations is below this.

### `initial_mesh_size`

`double`, default `0`.

Starting mesh size; 0 chooses it from the start point.

### `initial_frame_size`

`double`, default `0`.

Starting frame (poll) size; 0 chooses it from the start point.

### `mesh_refine_factor`

`double`, default `0.25`.

Mesh multiplier after an unsuccessful iteration.

### `frame_refine_factor`

`double`, default `0.5`.

Frame multiplier after an unsuccessful iteration.

### `mesh_expand_factor`

`double`, default `4`.

Mesh multiplier after a success.

### `frame_expand_factor`

`double`, default `2`.

Frame multiplier after a success.

### `mesh_tolerance`

`double`, default `1e-8`.

Converged when the frame size is at most this.

### `min_improvement`

`double`, default `0`.

A trial point must improve by more than this.

### `opportunistic`

`bool`, default `true`.

Stop a poll at the first improving point.

### `poll_scheme`

`dfo::MadsPollScheme`, default `Ortho2N`.

How the poll directions are built.

### `lower_bounds`

`pluss::Series`, default `undefined`.

Lower bound per variable.

### `upper_bounds`

`pluss::Series`, default `undefined`.

Upper bound per variable.

### `rng_seed`

`std::uint64_t`, default `5489`.

Seed of the random directions and search points.

### `promote_cpu_to_float64`

`bool`, default `true`.

Compute single-precision CPU inputs in double precision.

### `display`

`dfo::BlackboxSolverDisplay`, default `None`.

Print a line per iteration when `Iter`.

## Guidance

Passed to `mesh_adaptive_direct_search_minimizer`, `orthomads_minimizer` and `ltmads_minimizer`.

## Practical Examples

### Example 1

The poll scheme.

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

  for (const auto scheme : {dfo::MadsPollScheme::Ortho2N, dfo::MadsPollScheme::LtMadsRandom2N}) {
    dfo::MadsBlackboxConfig config;
    config.poll_scheme = scheme;
    const dfo::BlackboxSolverOutcome o = dfo::mesh_adaptive_direct_search_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config);
    out.writeln(scheme == dfo::MadsPollScheme::Ortho2N ? "Ortho2N       " : "LtMadsRandom2N", ": evaluations =", o.diagnostics.evaluations, ", minimum =", o.minimum);
  }
}
```

Output

```text
Ortho2N        : evaluations = 1000 , minimum = 0.00853561
LtMadsRandom2N : evaluations = 1000 , minimum = 0.00494886
```

### Example 2

Opportunistic polling off.

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

  for (const bool opportunistic : {true, false}) {
    dfo::MadsBlackboxConfig config;
    config.opportunistic = opportunistic;
    const dfo::BlackboxSolverOutcome o = dfo::orthomads_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config);
    out.writeln("opportunistic", opportunistic, ": evaluations =", o.diagnostics.evaluations, ", minimum =", o.minimum);
  }
}
```

Output

```text
opportunistic true : evaluations = 1000 , minimum = 0.00853561
opportunistic false : evaluations = 1000 , minimum = 0.01800498
```

### Example 3

The search step. Space-filling trial points before each poll find descent the poll is slow to find.

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

  for (const std::int64_t points : {0, 20}) {
    dfo::MadsBlackboxConfig config;
    config.search_points = points;
    config.max_evaluations = 1500;
    out.writeln("search points", points, ": minimum =", dfo::orthomads_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config).minimum);
  }
}
```

Output

```text
search points 0 : minimum = 0.00163321
search points 20 : minimum = 0.00000000
```

### Example 4

Refining the frame more slowly.

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

  for (const double refine : {0.5, 0.8}) {
    dfo::MadsBlackboxConfig config;
    config.frame_refine_factor = refine;
    config.max_evaluations = 1500;
    out.writeln("frame refine", refine, ": minimum =", dfo::orthomads_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config).minimum);
  }
}
```

Output

```text
frame refine 0.50000000 : minimum = 0.00163321
frame refine 0.80000000 : minimum = 0.00000066
```

## See Also

- [`mesh_adaptive_direct_search_minimizer`](mesh_adaptive_direct_search_minimizer.md): uses it
- [`MadsBlackboxAdvancedConfig`](MadsBlackboxAdvancedConfig.md): the advanced configuration

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="MadsBlackboxAdvancedConfig.html">
    <span class="label">Previous</span>
    <span class="title">MadsBlackboxAdvancedConfig</span>
  </a>
  <a class="next-link" href="MadsPollScheme.html">
    <span class="label">Next</span>
    <span class="title">MadsPollScheme</span>
  </a>
</div>
<!-- apiref:nav:end -->
