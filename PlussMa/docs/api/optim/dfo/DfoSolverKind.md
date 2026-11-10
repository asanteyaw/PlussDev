<!-- generated from DfoSolverKind.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::DfoSolverKind`

Which derivative-free method the front door runs.

`Auto` (the default) lets the front door choose from the shape of the problem; every other value forces a method. The choice rule is: a black-box callback selects `MadsAdvanced`; nonlinear constraints select `PatternSearch`; an unconstrained problem of at most `auto_policy.low_dimension_threshold` variables (10) selects `NelderMead`; an unconstrained problem between the thresholds selects `DifferentialEvolutionPrmm`; an unconstrained problem of at least `high_dimension_threshold` variables (40) selects `ParticleSwarm`; a boxed problem selects `DifferentialEvolution`, or `ParticleSwarm` from 40 variables. The reason for the choice is returned in `DfoResult::selection_reason`.

`NelderMead` is a simplex local search for small smooth-ish problems. `PatternSearch` polls a pattern of directions on a mesh and supports bounds, linear rows and nonlinear constraints. `DifferentialEvolution` and `DifferentialEvolutionPrmm` are population methods that need a box or a start to sample around. `ParticleSwarm` and `ParticleSwarmDv` are swarm methods. `OrthoMads` and `LtMads` are mesh-adaptive direct searches with different poll directions, and `MadsAdvanced` is the black-box MADS with constraints, several objectives, a cache and an optional surrogate. Only `PatternSearch` and the three MADS kinds accept nonlinear constraints.

## Fields

### `Auto`

`enumerator`, default `0`.

Choose from the problem: see the rule above.

### `NelderMead`

`enumerator`, default `1`.

Simplex local search; no constraints.

### `PatternSearch`

`enumerator`, default `2`.

Mesh polling with bounds, linear rows and nonlinear constraints.

### `DifferentialEvolution`

`enumerator`, default `3`.

Evolutionary population search.

### `DifferentialEvolutionPrmm`

`enumerator`, default `4`.

Differential evolution with adaptive parameters and elite reduction.

### `ParticleSwarm`

`enumerator`, default `5`.

Swarm search with inertia and acceleration schedules.

### `ParticleSwarmDv`

`enumerator`, default `6`.

Swarm search with differential velocity updates.

### `OrthoMads`

`enumerator`, default `7`.

Mesh-adaptive search with orthogonal poll directions.

### `LtMads`

`enumerator`, default `8`.

Mesh-adaptive search with lower-triangular random poll directions.

### `MadsAdvanced`

`enumerator`, default `9`.

Black-box mesh-adaptive search: constraints, several objectives, cache, surrogate.

## Guidance

Set as `DfoOptions::solver`; read back as `DfoResult::selected_solver`.

## Practical Examples

### Example 1

Every value has a name. `to_string` returns it, and the same names appear in `selected_solver` of a result.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  for (const dfo::DfoSolverKind kind : {dfo::DfoSolverKind::Auto, dfo::DfoSolverKind::NelderMead, dfo::DfoSolverKind::PatternSearch, dfo::DfoSolverKind::DifferentialEvolution,
                                        dfo::DfoSolverKind::DifferentialEvolutionPrmm, dfo::DfoSolverKind::ParticleSwarm, dfo::DfoSolverKind::ParticleSwarmDv,
                                        dfo::DfoSolverKind::OrthoMads, dfo::DfoSolverKind::LtMads, dfo::DfoSolverKind::MadsAdvanced})
    out.writeln(static_cast<int>(kind), dfo::to_string(kind));
}
```

Output

```text
0 Auto
1 NelderMead
2 PatternSearch
3 DifferentialEvolution
4 DifferentialEvolutionPrmm
5 ParticleSwarm
6 ParticleSwarmDv
7 OrthoMads
8 LtMads
9 MadsAdvanced
```

### Example 2

Forcing a method. The same quadratic is solved by three different kinds; each reports which one ran and the reason.

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

  for (const dfo::DfoSolverKind kind : {dfo::DfoSolverKind::NelderMead, dfo::DfoSolverKind::PatternSearch, dfo::DfoSolverKind::OrthoMads}) {
    dfo::DfoOptions options;
    options.solver = kind;
    const dfo::DfoResult result = dfo::solve_dfo(f, pluss::series::make({0.0, 0.0}), options);
    out.writeln(dfo::to_string(result.selected_solver), "|", result.selection_reason, "| minimum =", result.minimum);
  }
}
```

Output

```text
NelderMead | explicit solver override | minimum = 0.000000
PatternSearch | explicit solver override | minimum = 0.000000
OrthoMads | explicit solver override | minimum = 0.000000
```

### Example 3

What `Auto` chooses. A two-variable problem goes to Nelder-Mead, a boxed one to differential evolution, a constrained one to pattern search and a black box to MADS.

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
  const dfo::DfoResult plain = dfo::solve_dfo(f, start);
  const dfo::DfoResult boxed = dfo::DFO::build(start).with_objective(f).with_bounds(pluss::series::make({0.0, 0.0}), pluss::series::make({2.0, 2.0})).solve();
  const dfo::DfoResult constrained = dfo::DFO::build(start).with_objective(f).with_constraints([](const Series &x) { return x.sum() - 2.0; }).solve();
  const dfo::DfoResult black_box = dfo::DFO::build(start).with_blackbox([&](const Series &x) { return f(x); }).solve();
  for (const dfo::DfoResult *r : {&plain, &boxed, &constrained, &black_box}) out.writeln(dfo::to_string(r->selected_solver), "|", r->selection_reason);
}
```

Output

```text
NelderMead | small unconstrained problem; simplex local search is preferred
DifferentialEvolution | boxed problem without explicit constraints; differential evolution is preferred
PatternSearch | explicit nonlinear constraints detected
MadsAdvanced | blackbox callback provided
```

## See Also

- [`solve_dfo`](solve_dfo.md): the shortest call
- [`DfoOptions`](DfoOptions.md): the settings
- [`DfoAutoPolicy`](DfoAutoPolicy.md): the thresholds of `Auto`

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="DfoResult.html">
    <span class="label">Previous</span>
    <span class="title">DfoResult</span>
  </a>
  <a class="next-link" href="DfoStatus.html">
    <span class="label">Next</span>
    <span class="title">DfoStatus</span>
  </a>
</div>
<!-- apiref:nav:end -->
