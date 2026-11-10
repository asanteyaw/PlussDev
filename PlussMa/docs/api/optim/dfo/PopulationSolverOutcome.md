<!-- generated from PopulationSolverOutcome.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::PopulationSolverOutcome`

The result of a differential-evolution or swarm run.

`minimizer` is the best member and `minimum` its value; `status` is a [`PopulationSolverStatus`](PopulationSolverStatus.md) and `diagnostics` a [`PopulationSolverDiagnostics`](PopulationSolverDiagnostics.md). The minimizer is always inside the bounds when bounds were given.

## Fields

### `minimizer`

`pluss::Series`, default `undefined`.

The best point.

### `minimum`

`double`, default `0`.

The objective at the best point.

### `status`

`dfo::PopulationSolverStatus`, default `ReachedIterationBudget`.

Why the run stopped.

### `diagnostics`

`dfo::PopulationSolverDiagnostics`, default `{}`.

Counters.

## Guidance

Returned by the four population minimizers.

## Practical Examples

### Example 1

Reading an outcome.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto rastrigin = [](const Series &x) {
    const double pi = 3.14159265358979323846;
    return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum();   // minimum 0 at the origin, many local minima
  };

  dfo::DifferentialEvolutionConfig config;
  config.lower_bounds = pluss::series::make({-5.12, -5.12, -5.12});
  config.upper_bounds = pluss::series::make({5.12, 5.12, 5.12});
  config.max_evaluations = 20000;
  const dfo::PopulationSolverOutcome o = dfo::differential_evolution_minimizer(rastrigin, pluss::series::make({3.0, 3.0, 3.0}), config);
  out.writeln("minimizer =", o.minimizer.items<double>(), ", minimum =", o.minimum, ", status =", dfo::to_string(o.status));
}
```

Output

```text
minimizer = [-0.024462, 0.012126, 0.012378] , minimum = 0.178024 , status = ReachedEvaluationBudget
```

### Example 2

The bounds hold. A minimum outside the box is clamped to its edge.

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

  dfo::ParticleSwarmConfig config;
  config.lower_bounds = pluss::series::make({0.0, 0.0});
  config.upper_bounds = pluss::series::make({2.0, 2.0});
  config.max_evaluations = 4000;
  const dfo::PopulationSolverOutcome o = dfo::particle_swarm_minimizer(f, pluss::series::make({0.0, 0.0}), config);
  out.writeln("minimizer =", o.minimizer.items<double>());
}
```

Output

```text
minimizer = [2.0000, 2.0000]
```

### Example 3

Comparing the four methods.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto rastrigin = [](const Series &x) {
    const double pi = 3.14159265358979323846;
    return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum();   // minimum 0 at the origin, many local minima
  };

  const Series start = pluss::series::make({3.0, 3.0, 3.0});
  const Series lo = pluss::series::make({-5.12, -5.12, -5.12}), hi = pluss::series::make({5.12, 5.12, 5.12});
  dfo::DifferentialEvolutionConfig a; a.lower_bounds = lo; a.upper_bounds = hi; a.max_evaluations = 8000;
  dfo::DifferentialEvolutionPrmmConfig b; b.lower_bounds = lo; b.upper_bounds = hi; b.max_evaluations = 8000;
  dfo::ParticleSwarmConfig c; c.lower_bounds = lo; c.upper_bounds = hi; c.max_evaluations = 8000;
  dfo::ParticleSwarmDvConfig d; d.lower_bounds = lo; d.upper_bounds = hi; d.max_evaluations = 8000;
  out.writeln("DE      :", dfo::differential_evolution_minimizer(rastrigin, start, a).minimum);
  out.writeln("DE-PRMM :", dfo::differential_evolution_prmm_minimizer(rastrigin, start, b).minimum);
  out.writeln("PSO     :", dfo::particle_swarm_minimizer(rastrigin, start, c).minimum);
  out.writeln("PSO-DV  :", dfo::particle_swarm_dv_minimizer(rastrigin, start, d).minimum);
}
```

Output

```text
DE      : 1.136888
DE-PRMM : 0.000000
PSO     : 0.000000
PSO-DV  : 9.909159
```

## See Also

- [`differential_evolution_minimizer`](differential_evolution_minimizer.md): produces it
- [`PopulationSolverDiagnostics`](PopulationSolverDiagnostics.md): the counters

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="PopulationSolverExecutionMode.html">
    <span class="label">Previous</span>
    <span class="title">PopulationSolverExecutionMode</span>
  </a>
  <a class="next-link" href="PopulationSolverStatus.html">
    <span class="label">Next</span>
    <span class="title">PopulationSolverStatus</span>
  </a>
</div>
<!-- apiref:nav:end -->
