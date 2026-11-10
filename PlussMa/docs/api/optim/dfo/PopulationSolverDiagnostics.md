<!-- generated from PopulationSolverDiagnostics.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::PopulationSolverDiagnostics`

The counters of a differential-evolution or swarm run.

`iterations` counts generations, `evaluations` objective calls (the population size times the generations, plus the start and any centre particle), and `last_step` names the last step the method took.

## Fields

### `iterations`

`std::int64_t`, default `0`.

Generations performed.

### `evaluations`

`std::int64_t`, default `0`.

Objective evaluations.

### `last_step`

`std::string`, default `empty`.

Name of the last step.

## Guidance

Read from `PopulationSolverOutcome::diagnostics`.

## Practical Examples

### Example 1

A swarm run.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto rastrigin = [](const Series &x) {
    const double pi = 3.14159265358979323846;
    return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum();   // minimum 0 at the origin, many local minima
  };

  dfo::ParticleSwarmConfig config;
  config.lower_bounds = pluss::series::make({-5.12, -5.12, -5.12});
  config.upper_bounds = pluss::series::make({5.12, 5.12, 5.12});
  config.max_evaluations = 4000;
  const dfo::PopulationSolverOutcome o = dfo::particle_swarm_minimizer(rastrigin, pluss::series::make({3.0, 3.0, 3.0}), config);
  out.writeln("generations =", o.diagnostics.iterations, ", evaluations =", o.diagnostics.evaluations, ", last step =", o.diagnostics.last_step);
}
```

Output

```text
generations = 19 , evaluations = 4019 , last step = generation
```

### Example 2

Evaluations per generation equal the population size.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto rastrigin = [](const Series &x) {
    const double pi = 3.14159265358979323846;
    return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum();   // minimum 0 at the origin, many local minima
  };

  dfo::DifferentialEvolutionConfig config;
  config.population_size = 50;
  config.max_generations = 10;
  config.lower_bounds = pluss::series::make({-5.12, -5.12, -5.12});
  config.upper_bounds = pluss::series::make({5.12, 5.12, 5.12});
  const dfo::PopulationSolverOutcome o = dfo::differential_evolution_minimizer(rastrigin, pluss::series::make({3.0, 3.0, 3.0}), config);
  out.writeln("generations =", o.diagnostics.iterations, ", evaluations =", o.diagnostics.evaluations);
}
```

Output

```text
generations = 10 , evaluations = 550
```

### Example 3

The adaptive variant.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto rastrigin = [](const Series &x) {
    const double pi = 3.14159265358979323846;
    return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum();   // minimum 0 at the origin, many local minima
  };

  dfo::DifferentialEvolutionPrmmConfig config;
  config.lower_bounds = pluss::series::make({-5.12, -5.12, -5.12});
  config.upper_bounds = pluss::series::make({5.12, 5.12, 5.12});
  config.max_evaluations = 4000;
  const dfo::PopulationSolverOutcome o = dfo::differential_evolution_prmm_minimizer(rastrigin, pluss::series::make({3.0, 3.0, 3.0}), config);
  out.writeln("generations =", o.diagnostics.iterations, ", evaluations =", o.diagnostics.evaluations);
}
```

Output

```text
generations = 41 , evaluations = 3700
```

## See Also

- [`PopulationSolverOutcome`](PopulationSolverOutcome.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ParticleSwarmDvConfig.html">
    <span class="label">Previous</span>
    <span class="title">ParticleSwarmDvConfig</span>
  </a>
  <a class="next-link" href="PopulationSolverDisplay.html">
    <span class="label">Next</span>
    <span class="title">PopulationSolverDisplay</span>
  </a>
</div>
<!-- apiref:nav:end -->
