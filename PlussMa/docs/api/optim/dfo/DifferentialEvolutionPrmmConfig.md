<!-- generated from DifferentialEvolutionPrmmConfig.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::DifferentialEvolutionPrmmConfig`

Population, self-adaptation, reduction and budget settings of adaptive differential evolution.

`initial_mutation_factor` and `initial_crossover_rate` seed each member's own `F` and `CR`; `tau_mutation` and `tau_crossover` are the probabilities that a member re-draws them, `F` uniformly from `[mutation_lower, mutation_upper]`. `elite_count` is the number of best members used by the elite strategies, `random_main_strategy_probability` the chance of the random-base strategy, and `diversity_epsilon` the threshold of the diversity test that decides whether the leading member is refreshed. `reduction_passes` is how many times the population is halved, and `pmax` sets the generation budget as `max_evaluations / (pmax × population_size)` when `max_generations` is 0. The evaluation limit defaults to 100000.

## Fields

### `population_size`

`std::int64_t`, default `200`.

Starting population (halved by each reduction pass).

### `elite_count`

`std::int64_t`, default `6`.

Best members used by the elite strategies.

### `max_generations`

`std::int64_t`, default `0`.

Generation limit; 0 derives it from the evaluation budget.

### `max_evaluations`

`std::int64_t`, default `100000`.

Evaluation limit.

### `check_every`

`std::int64_t`, default `0`.

Generations between improvement tests; 0 disables the test.

### `reduction_passes`

`std::int64_t`, default `3`.

How many times the population is halved.

### `pmax`

`std::int64_t`, default `4`.

Budget partition: generations = evaluations / (pmax × population).

### `relative_improvement_tolerance`

`double`, default `1e-8`.

Stop when the relative improvement is below this.

### `initial_mutation_factor`

`double`, default `0.8`.

Starting `F` of every member.

### `initial_crossover_rate`

`double`, default `0.9`.

Starting `CR` of every member.

### `mutation_lower`

`double`, default `0.1`.

Lower end of the re-drawn `F`.

### `mutation_upper`

`double`, default `1`.

Upper end of the re-drawn `F`.

### `tau_mutation`

`double`, default `0.1`.

Probability a member re-draws its `F`.

### `tau_crossover`

`double`, default `0.1`.

Probability a member re-draws its `CR`.

### `random_main_strategy_probability`

`double`, default `0.75`.

Probability of the random-base strategy.

### `diversity_epsilon`

`double`, default `0.5`.

Threshold of the diversity test for refreshing the leader.

### `initial_radius`

`double`, default `+inf`.

Half-width of the sampling box around the start; infinity means the whole box when bounded, otherwise `start ± 0.5`.

### `lower_bounds`

`pluss::Series`, default `undefined`.

Lower bound per variable; set together with `upper_bounds`.

### `upper_bounds`

`pluss::Series`, default `undefined`.

Upper bound per variable.

### `rng_seed`

`std::uint64_t`, default `5489`.

Seed of the random numbers.

### `promote_cpu_to_float64`

`bool`, default `true`.

Compute single-precision CPU inputs in double precision.

### `display`

`dfo::PopulationSolverDisplay`, default `None`.

Print a line per generation when `Iter`.

### `execution_mode`

`dfo::PopulationSolverExecutionMode`, default `Vectorized`.

How a generation is computed.

## Guidance

Passed to `differential_evolution_prmm_minimizer`, or set as `DfoOptions::differential_evolution_prmm`.

## Practical Examples

### Example 1

The reduction passes. More passes halve the population more times and spend the late budget on a smaller, refining population.

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

  for (const std::int64_t passes : {0, 1, 3}) {
    dfo::DifferentialEvolutionPrmmConfig config;
    config.reduction_passes = passes;
    config.lower_bounds = pluss::series::make({-5.12, -5.12, -5.12});
    config.upper_bounds = pluss::series::make({5.12, 5.12, 5.12});
    config.max_evaluations = 20000;
    const dfo::PopulationSolverOutcome o = dfo::differential_evolution_prmm_minimizer(rastrigin, pluss::series::make({3.0, 3.0, 3.0}), config);
    out.writeln("passes", passes, ": evaluations =", o.diagnostics.evaluations, ", minimum =", o.minimum);
  }
}
```

Output

```text
passes 0 : evaluations = 5400 , minimum = 1.99641569
passes 1 : evaluations = 10000 , minimum = 0.00000902
passes 3 : evaluations = 19700 , minimum = 0.00000000
```

### Example 2

The population size.

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

  for (const std::int64_t n : {40, 200}) {
    dfo::DifferentialEvolutionPrmmConfig config;
    config.population_size = n;
    config.lower_bounds = pluss::series::make({-5.12, -5.12, -5.12});
    config.upper_bounds = pluss::series::make({5.12, 5.12, 5.12});
    config.max_evaluations = 20000;
    const dfo::PopulationSolverOutcome o = dfo::differential_evolution_prmm_minimizer(rastrigin, pluss::series::make({3.0, 3.0, 3.0}), config);
    out.writeln("population", n, ": evaluations =", o.diagnostics.evaluations, ", minimum =", o.minimum);
  }
}
```

Output

```text
population 40 : evaluations = 19940 , minimum = 0.00000000
population 200 : evaluations = 19700 , minimum = 0.00000000
```

### Example 3

The elite size and the adaptation probability.

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

  for (const std::int64_t elite : {2, 12}) {
    dfo::DifferentialEvolutionPrmmConfig config;
    config.elite_count = elite;
    config.tau_mutation = 0.3;
    config.lower_bounds = pluss::series::make({-5.12, -5.12, -5.12});
    config.upper_bounds = pluss::series::make({5.12, 5.12, 5.12});
    config.max_evaluations = 20000;
    out.writeln("elites", elite, ": minimum =", dfo::differential_evolution_prmm_minimizer(rastrigin, pluss::series::make({3.0, 3.0, 3.0}), config).minimum);
  }
}
```

Output

```text
elites 2 : minimum = 0.00000000
elites 12 : minimum = 0.00000000
```

## See Also

- [`differential_evolution_prmm_minimizer`](differential_evolution_prmm_minimizer.md): uses it
- [`DfoOptions`](DfoOptions.md): holds one

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="DifferentialEvolutionConfig.html">
    <span class="label">Previous</span>
    <span class="title">DifferentialEvolutionConfig</span>
  </a>
  <a class="next-link" href="DiscoMadsConfig.html">
    <span class="label">Next</span>
    <span class="title">DiscoMadsConfig</span>
  </a>
</div>
<!-- apiref:nav:end -->
