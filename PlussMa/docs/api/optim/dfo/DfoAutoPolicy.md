<!-- generated from DfoAutoPolicy.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::DfoAutoPolicy`

The dimension thresholds `Auto` uses to choose among the unconstrained and boxed methods.

Read when `DfoOptions::solver` is `Auto`, as `options.auto_policy`. An unconstrained problem with at most `low_dimension_threshold` variables goes to Nelder-Mead; one with at least `high_dimension_threshold` variables goes to the swarm method when `prefer_pso_for_high_dimension` is set; between the two, differential evolution with parameter adaptation is used. A boxed problem goes to differential evolution, or to the swarm from the high threshold. A black box or a constrained problem is routed by its kind and ignores the thresholds.

## Fields

### `low_dimension_threshold`

`std::int64_t`, default `10`.

Unconstrained problems with at most this many variables go to Nelder-Mead.

### `high_dimension_threshold`

`std::int64_t`, default `40`.

Problems with at least this many variables go to the swarm, when the preference is on.

### `prefer_pso_for_high_dimension`

`bool`, default `true`.

Whether the high-dimension route is the swarm; when false the evolutionary route is used instead.

## Guidance

Set as `DfoOptions::auto_policy`.

## Practical Examples

### Example 1

Moving the low threshold. A four-variable problem normally goes to Nelder-Mead; with the threshold at 2 it is treated as medium-dimensional.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x - pluss::series::make({1.0, -2.0, 0.5, 3.0})).square().sum(); };   // minimum 0 at (1, -2, 0.5, 3)

  const Series start = pluss::series::make({0.0, 0.0, 0.0, 0.0});
  for (const std::int64_t low : {10, 2}) {
    dfo::DfoOptions options;
    options.auto_policy.low_dimension_threshold = low;
    const dfo::DfoResult r = dfo::solve_dfo(f, start, options);
    out.writeln("low threshold", low, ":", dfo::to_string(r.selected_solver));
  }
}
```

Output

```text
low threshold 10 : NelderMead
low threshold 2 : DifferentialEvolutionPrmm
```

### Example 2

The high threshold. With the threshold at 3 the same unconstrained problem goes to the swarm.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x - pluss::series::make({1.0, -2.0, 0.5, 3.0})).square().sum(); };   // minimum 0 at (1, -2, 0.5, 3)

  dfo::DfoOptions options;
  options.auto_policy.low_dimension_threshold = 2;
  options.auto_policy.high_dimension_threshold = 3;
  const dfo::DfoResult r = dfo::solve_dfo(f, pluss::series::make({0.0, 0.0, 0.0, 0.0}), options);
  out.writeln(dfo::to_string(r.selected_solver), "|", r.selection_reason);
}
```

Output

```text
ParticleSwarm | high-dimensional unconstrained problem; swarm search is preferred
```

### Example 3

Turning the swarm preference off. With `prefer_pso_for_high_dimension = false` a problem above the high threshold keeps the differential-evolution route.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x - pluss::series::make({1.0, -2.0, 0.5, 3.0})).square().sum(); };   // minimum 0 at (1, -2, 0.5, 3)

  for (const bool prefer : {true, false}) {
    dfo::DfoOptions options;
    options.auto_policy.low_dimension_threshold = 2;
    options.auto_policy.high_dimension_threshold = 3;
    options.auto_policy.prefer_pso_for_high_dimension = prefer;
    const dfo::DfoResult r = dfo::solve_dfo(f, pluss::series::make({0.0, 0.0, 0.0, 0.0}), options);
    out.writeln("prefer swarm =", prefer, ":", dfo::to_string(r.selected_solver));
  }
}
```

Output

```text
prefer swarm = true : ParticleSwarm
prefer swarm = false : DifferentialEvolutionPrmm
```

## See Also

- [`DfoOptions`](DfoOptions.md): holds it
- [`DfoSolverKind`](DfoSolverKind.md): the rule

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="DFO.html">
    <span class="label">Previous</span>
    <span class="title">DFO</span>
  </a>
  <a class="next-link" href="DfoBuilder.html">
    <span class="label">Next</span>
    <span class="title">DfoBuilder</span>
  </a>
</div>
<!-- apiref:nav:end -->
