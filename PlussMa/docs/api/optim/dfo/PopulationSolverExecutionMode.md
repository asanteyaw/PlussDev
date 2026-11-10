<!-- generated from PopulationSolverExecutionMode.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::PopulationSolverExecutionMode`

How a generation of a population method is computed.

`Vectorized` (the default) computes a whole generation as array operations on the population, which is the fast path. `OptimMasterCompat` updates the members one after another in the classic sequential order, where each member can see the updates already made in the same generation; it follows a different random path and is slower, and is useful when a result must be reproduced from a sequential formulation. Both are valid; a given seed gives a different run in each.

## Fields

### `Vectorized`

`enumerator`, default `0`.

Whole-generation array operations.

### `OptimMasterCompat`

`enumerator`, default `1`.

Member-by-member sequential updates.

## Guidance

Set as `execution_mode` in a population configuration.

## Practical Examples

### Example 1

The two modes on one problem.

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

  for (const auto mode : {dfo::PopulationSolverExecutionMode::Vectorized, dfo::PopulationSolverExecutionMode::OptimMasterCompat}) {
    dfo::DifferentialEvolutionConfig config;
    config.execution_mode = mode;
    config.lower_bounds = pluss::series::make({-5.12, -5.12, -5.12});
    config.upper_bounds = pluss::series::make({5.12, 5.12, 5.12});
    config.max_evaluations = 8000;
    const dfo::PopulationSolverOutcome o = dfo::differential_evolution_minimizer(rastrigin, pluss::series::make({3.0, 3.0, 3.0}), config);
    out.writeln(static_cast<int>(mode), ": evaluations =", o.diagnostics.evaluations, ", minimum =", o.minimum);
  }
}
```

Output

```text
0 : evaluations = 8000 , minimum = 1.136888
1 : evaluations = 8000 , minimum = 2.245459
```

### Example 2

Each mode is repeatable by its seed.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto rastrigin = [](const Series &x) {
    const double pi = 3.14159265358979323846;
    return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum();   // minimum 0 at the origin, many local minima
  };

  for (const std::uint64_t seed : {1, 1}) {
    dfo::DifferentialEvolutionConfig config;
    config.execution_mode = dfo::PopulationSolverExecutionMode::OptimMasterCompat;
    config.rng_seed = seed;
    config.lower_bounds = pluss::series::make({-5.12, -5.12, -5.12});
    config.upper_bounds = pluss::series::make({5.12, 5.12, 5.12});
    config.max_evaluations = 4000;
    out.writeln("seed", seed, ": minimum =", dfo::differential_evolution_minimizer(rastrigin, pluss::series::make({3.0, 3.0, 3.0}), config).minimum);
  }
}
```

Output

```text
seed 1 : minimum = 5.64296065
seed 1 : minimum = 5.64296065
```

### Example 3

On a swarm.

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

  for (const auto mode : {dfo::PopulationSolverExecutionMode::Vectorized, dfo::PopulationSolverExecutionMode::OptimMasterCompat}) {
    dfo::ParticleSwarmConfig config;
    config.execution_mode = mode;
    config.lower_bounds = pluss::series::make({-5.12, -5.12, -5.12});
    config.upper_bounds = pluss::series::make({5.12, 5.12, 5.12});
    config.max_evaluations = 4000;
    out.writeln(static_cast<int>(mode), ": minimum =", dfo::particle_swarm_minimizer(rastrigin, pluss::series::make({3.0, 3.0, 3.0}), config).minimum);
  }
}
```

Output

```text
0 : minimum = 0.000008
1 : minimum = 0.000050
```

## See Also

- [`DifferentialEvolutionConfig`](DifferentialEvolutionConfig.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="PopulationSolverDisplay.html">
    <span class="label">Previous</span>
    <span class="title">PopulationSolverDisplay</span>
  </a>
  <a class="next-link" href="PopulationSolverOutcome.html">
    <span class="label">Next</span>
    <span class="title">PopulationSolverOutcome</span>
  </a>
</div>
<!-- apiref:nav:end -->
