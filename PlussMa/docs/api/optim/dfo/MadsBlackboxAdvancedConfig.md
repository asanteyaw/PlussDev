<!-- generated from MadsBlackboxAdvancedConfig.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::MadsBlackboxAdvancedConfig`

The black-box mesh-adaptive settings: everything in the basic configuration plus constraints, several objectives, cache, surrogate and Pareto archive.

It extends [`MadsBlackboxConfig`](MadsBlackboxConfig.md), so every basic field is present too. The additions are: **constraint handling** ([`ConstraintHandling`](ConstraintHandling.md)), the **scalarisation** of several objectives ([`MultiObjectiveScalarization`](MultiObjectiveScalarization.md)) with `objective_weights` and `objective_reference`, the **cache** of evaluated points, the evaluation **counters and budgets** (`max_total_evaluations`, `count_cache_hits_in_total_evaluations`), the **surrogate** that orders poll candidates ([`SurrogateMode`](SurrogateMode.md)), and the **Pareto archive** of non-dominated points for several objectives. This is the configuration read by the front door as `DfoOptions::mads`.

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

### `constraint_handling`

`dfo::ConstraintHandling`, default `Ignore`.

What is done with the constraint values.

### `constraint_tolerance`

`double`, default `1e-12`.

A violation up to this counts as feasible.

### `progressive_barrier_penalty`

`double`, default `100`.

Penalty per unit of violation of the progressive barrier.

### `multiobjective_scalarization`

`dfo::MultiObjectiveScalarization`, default `WeightedSum`.

How several objectives become one.

### `objective_weights`

`pluss::Series`, default `undefined`.

Weights of the objectives.

### `objective_reference`

`pluss::Series`, default `undefined`.

Reference point of the Chebyshev scalarisation.

### `enable_cache`

`bool`, default `true`.

Answer repeated points from a cache.

### `cache_max_entries`

`std::int64_t`, default `200000`.

Cache capacity.

### `cache_round_digits`

`std::int64_t`, default `12`.

Points are rounded to this many digits before lookup.

### `count_cache_hits_in_total_evaluations`

`bool`, default `true`.

Whether a cache hit counts toward `max_total_evaluations`.

### `max_total_evaluations`

`std::int64_t`, default `0`.

Budget on all calls including cache hits and free calls; 0 uses 4 × the evaluation limit.

### `surrogate_mode`

`dfo::SurrogateMode`, default `Off`.

Whether a model orders the poll candidates.

### `surrogate_min_points`

`std::int64_t`, default `24`.

Evaluated points needed before the model is used.

### `surrogate_max_points`

`std::int64_t`, default `512`.

Evaluated points kept for the model.

### `surrogate_bandwidth`

`double`, default `0`.

Smoothing length of the model; 0 chooses it automatically.

### `accelerate_heavy_kernels`

`bool`, default `true`.

Run large array computations on a faster device when available.

### `auto_heavy_platform`

`bool`, default `true`.

Choose that device automatically.

### `heavy_platform`

`pluss::Platform`, default `CPU`.

The device for those computations when not automatic.

### `heavy_dtype`

`pluss::ScalarType`, default `Float32`.

The precision of those computations.

### `pareto_epsilon`

`double`, default `1e-12`.

Tolerance of the dominance test in the Pareto archive.

### `pareto_archive_limit`

`std::int64_t`, default `256`.

Most points kept in the Pareto archive.

## Guidance

Passed to the black-box minimizers, or set as `DfoOptions::mads`.

## Practical Examples

### Example 1

The cache. A kink in the objective makes poll points repeat; with the cache on, repeats are answered from memory.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  for (const bool cache : {true, false}) {
    std::int64_t calls = 0;
    const auto black_box = [&](const Series &x) {
      ++calls;
      dfo::BlackboxEvaluation e;
      e.objectives = (x - pluss::series::make({1.0, 0.5})).abs().sum().reshape({1});
      return e;
    };
    dfo::MadsBlackboxAdvancedConfig config;
    config.enable_cache = cache;
    const dfo::BlackboxAdvancedOutcome o = dfo::mads_blackbox_minimizer(black_box, pluss::series::make({3.0, -2.0}), config);
    out.writeln("cache", cache, ": black-box calls =", calls, ", cache hits =", o.diagnostics.cache_hits);
  }
}
```

Output

```text
cache true : black-box calls = 457 , cache hits = 66
cache false : black-box calls = 664 , cache hits = 0
```

### Example 2

A budget on all calls. `max_total_evaluations` bounds counted and free calls and cache hits together.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto black_box = [](const Series &x) {
    dfo::BlackboxEvaluation e;
    e.objectives = (x - 3.0).square().sum().reshape({1});   // minimum 0 at (3, 3)
    return e;
  };

  dfo::MadsBlackboxAdvancedConfig config;
  config.max_total_evaluations = 60;
  const dfo::BlackboxAdvancedOutcome o = dfo::mads_blackbox_minimizer(black_box, pluss::series::make({0.0, 0.0}), config);
  out.writeln(dfo::to_string(o.status), ", evaluations =", o.diagnostics.evaluations, ", total =", o.diagnostics.total_evaluations);
}
```

Output

```text
ReachedEvaluationBudget , evaluations = 58 , total = 60
```

### Example 3

Constraint handling and a barrier penalty. The constraint `x₀ ≤ 1` on a black box, with the progressive barrier.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));
  const auto black_box = [](const Series &x) {
    dfo::BlackboxEvaluation e;
    e.objectives = (x - 3.0).square().sum().reshape({1});
    e.constraints = (x.select(0, 0) - 1.0).reshape({1});
    return e;
  };
  dfo::MadsBlackboxAdvancedConfig config;
  config.constraint_handling = dfo::ConstraintHandling::ProgressiveBarrier;
  config.progressive_barrier_penalty = 1000.0;
  const dfo::BlackboxAdvancedOutcome o = dfo::mads_blackbox_minimizer(black_box, pluss::series::make({0.0, 0.0}), config);
  out.writeln("minimizer =", o.minimizer.items<double>());
}
```

Output

```text
minimizer = [1.00, 3.00]
```

### Example 4

The Pareto archive limit. Two objectives `x²` and `(x − 2)²` on one variable have the whole interval `[0, 2]` as their trade-off; the archive keeps at most the limit.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto black_box = [](const Series &x) {
    dfo::BlackboxEvaluation e;
    const Series a = x.select(0, 0);
    e.objectives = a.square().reshape({1}).unite({(a - 2.0).square().reshape({1})}, 0);
    return e;
  };
  for (const std::int64_t limit : {256, 8}) {
    dfo::MadsBlackboxAdvancedConfig config;
    config.pareto_archive_limit = limit;
    config.objective_weights = pluss::series::make({0.5, 0.5});
    config.max_evaluations = 800;
    const dfo::BlackboxAdvancedOutcome o = dfo::mads_blackbox_minimizer(black_box, pluss::series::make({5.0}), config);
    out.writeln("limit", limit, ": archive size =", o.pareto_points.size(0));
  }
}
```

Output

```text
limit 256 : archive size = 73
limit 8 : archive size = 8
```

## See Also

- [`mads_blackbox_minimizer`](mads_blackbox_minimizer.md): uses it
- [`DfoOptions`](DfoOptions.md): holds one

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="DiscoMadsConfig.html">
    <span class="label">Previous</span>
    <span class="title">DiscoMadsConfig</span>
  </a>
  <a class="next-link" href="MadsBlackboxConfig.html">
    <span class="label">Next</span>
    <span class="title">MadsBlackboxConfig</span>
  </a>
</div>
<!-- apiref:nav:end -->
