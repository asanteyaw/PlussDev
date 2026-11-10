<!-- generated from differential_evolution_minimizer.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::differential_evolution_minimizer`

Minimises an objective with differential evolution, a population method that needs no gradient.

## Signature

```cpp
dfo::PopulationSolverOutcome dfo::differential_evolution_minimizer(const std::function<pluss::Series(const pluss::Series &)> &objective, const pluss::Series &initial, const dfo::DifferentialEvolutionConfig &config = {});
template <class Fn> dfo::PopulationSolverOutcome dfo::differential_evolution_minimizer(Fn &&objective, const pluss::Series &initial, const dfo::DifferentialEvolutionConfig &config = {});
```

## What It Does

Differential evolution keeps a population of candidate points. Each generation, every member is challenged by a **trial** point built from other members: a mutant `a + F (b − c)` from three random members (or blended toward the best when `best_mix` is positive), mixed with the member itself coordinate by coordinate with probability `crossover_rate`. The trial replaces the member if it is no worse. The population spreads over the search region, which makes the method robust against local minima where a local method such as Nelder-Mead would be trapped.

The sampling region is the box `[lower_bounds, upper_bounds]` when bounds are given, otherwise `initial ± initial_radius` (`± 0.5` by default). Bounds are respected by every member. The objective is evaluated on the whole population at once as array operations (the `Vectorized` execution mode), so a generation of 200 members is a single call's worth of work. It runs until the population stops improving (relative improvement below the tolerance, checked every `check_every` generations), `max_generations`, or `max_evaluations`.

## Parameters

`Fn &&objective`
: A callable `Series -> Series` (real scalar) or `Series -> number`.

`const pluss::Series &initial`
: The starting point; fixes the dimension and the sampling box when there are no bounds.

`const dfo::DifferentialEvolutionConfig &config`
: Population, mutation, crossover, budgets and bounds.

## Returns

A [`PopulationSolverOutcome`](PopulationSolverOutcome.md).

## Errors

- An objective that is not a real scalar throws.

## Practical Examples

### Example 1

A multimodal function. The Rastrigin function has a lattice of local minima around its global minimum 0 at the origin; the population ends close to the global one, within the evaluation budget.

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
  out.writeln("minimizer =", o.minimizer.items<double>(), ", minimum =", o.minimum);
  out.writeln("status =", dfo::to_string(o.status), ", generations =", o.diagnostics.iterations, ", evaluations =", o.diagnostics.evaluations);
}
```

Output

```text
minimizer = [-0.024462, 0.012126, 0.012378] , minimum = 0.178024
status = ReachedEvaluationBudget , generations = 99 , evaluations = 20000
```

### Example 2

Without bounds. The population is sampled around the start within `initial_radius`.

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
  config.initial_radius = 5.0;
  const dfo::PopulationSolverOutcome o = dfo::differential_evolution_minimizer(f, pluss::series::make({0.0, 0.0, 0.0, 0.0}), config);
  out.writeln("minimizer =", o.minimizer.items<double>(), ", minimum =", o.minimum);
}
```

Output

```text
minimizer = [1.0000, -2.0000, 0.5000, 3.0000] , minimum = 0.0000
```

### Example 3

A budget. With a small evaluation budget the run ends `ReachedEvaluationBudget` with the best point so far.

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
  config.lower_bounds = pluss::series::make({-5.12, -5.12, -5.12});
  config.upper_bounds = pluss::series::make({5.12, 5.12, 5.12});
  config.max_evaluations = 1000;
  const dfo::PopulationSolverOutcome o = dfo::differential_evolution_minimizer(rastrigin, pluss::series::make({3.0, 3.0, 3.0}), config);
  out.writeln(dfo::to_string(o.status), ", evaluations =", o.diagnostics.evaluations, ", minimum =", o.minimum);
}
```

Output

```text
ReachedEvaluationBudget , evaluations = 1000 , minimum = 11.8751
```

### Example 4

A non-smooth objective: the sum of absolute values, which has a kink at its minimum.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto f = [](const Series &x) { return (x - pluss::series::make({1.0, -2.0})).abs().sum(); };
  dfo::DifferentialEvolutionConfig config;
  config.lower_bounds = pluss::series::make({-5.0, -5.0});
  config.upper_bounds = pluss::series::make({5.0, 5.0});
  const dfo::PopulationSolverOutcome o = dfo::differential_evolution_minimizer(f, pluss::series::make({0.0, 0.0}), config);
  out.writeln("minimizer =", o.minimizer.items<double>(), ", minimum =", o.minimum);
}
```

Output

```text
minimizer = [1.00000, -2.00000] , minimum = 0.00000
```

## See Also

- [`DifferentialEvolutionConfig`](DifferentialEvolutionConfig.md): the settings
- [`differential_evolution_prmm_minimizer`](differential_evolution_prmm_minimizer.md): the adaptive variant

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="coopmads_blackbox_minimizer.html">
    <span class="label">Previous</span>
    <span class="title">coopmads_blackbox_minimizer</span>
  </a>
  <a class="next-link" href="differential_evolution_prmm_minimizer.html">
    <span class="label">Next</span>
    <span class="title">differential_evolution_prmm_minimizer</span>
  </a>
</div>
<!-- apiref:nav:end -->
