<!-- generated from PopulationSolverDisplay.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::PopulationSolverDisplay`

Whether a differential-evolution or swarm run prints its progress.

`None` is silent; `Iter` prints one line per generation to the standard output. Set as the `display` member of the four population configurations; through the front door use `DfoOptions::display_iter`.

## Fields

### `None`

`enumerator`, default `0`.

No output.

### `Iter`

`enumerator`, default `1`.

One line per generation.

## Guidance

Set as `display` in a population configuration.

## Practical Examples

### Example 1

Printing a short run.

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

  dfo::DifferentialEvolutionConfig config;
  config.display = dfo::PopulationSolverDisplay::Iter;
  config.max_generations = 3;
  config.population_size = 20;
  config.initial_radius = 5.0;
  dfo::differential_evolution_minimizer(f, pluss::series::make({0.0, 0.0, 0.0, 0.0}), config);
}
```

Output

```text
iter=0 evals=20 f=7.90371 step=initial_population
iter=1 evals=40 f=7.90371 step=generation
iter=2 evals=60 f=7.90371 step=generation
iter=3 evals=80 f=4.25727 step=generation
```

### Example 2

A swarm.

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

  dfo::ParticleSwarmConfig config;
  config.display = dfo::PopulationSolverDisplay::Iter;
  config.max_generations = 3;
  config.population_size = 20;
  config.initial_radius = 5.0;
  dfo::particle_swarm_minimizer(f, pluss::series::make({0.0, 0.0, 0.0, 0.0}), config);
}
```

Output

```text
iter=0 evals=21 f=7.90371 step=initial_population
iter=1 evals=42 f=5.47991 step=generation
iter=2 evals=63 f=5.47991 step=generation
iter=3 evals=83 f=5.22419 step=generation
```

### Example 3

The default is silent.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  out.writeln("default is None =", dfo::DifferentialEvolutionConfig{}.display == dfo::PopulationSolverDisplay::None);
}
```

Output

```text
default is None = true
```

## See Also

- [`DifferentialEvolutionConfig`](DifferentialEvolutionConfig.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="PopulationSolverDiagnostics.html">
    <span class="label">Previous</span>
    <span class="title">PopulationSolverDiagnostics</span>
  </a>
  <a class="next-link" href="PopulationSolverExecutionMode.html">
    <span class="label">Next</span>
    <span class="title">PopulationSolverExecutionMode</span>
  </a>
</div>
<!-- apiref:nav:end -->
