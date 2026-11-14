<!-- generated from particle_swarm_dv_minimizer.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::particle_swarm_dv_minimizer`

Minimises an objective with a swarm whose velocity is built from differences between particles.

## Signature

```cpp
dfo::PopulationSolverOutcome dfo::particle_swarm_dv_minimizer(const std::function<pluss::Series(const pluss::Series &)> &objective, const pluss::Series &initial, const dfo::ParticleSwarmDvConfig &config = {});
template <class Fn> dfo::PopulationSolverOutcome dfo::particle_swarm_dv_minimizer(Fn &&objective, const pluss::Series &initial, const dfo::ParticleSwarmDvConfig &config = {});
```

## What It Does

The differential-velocity swarm combines the two population ideas. A particle's velocity is built from the **difference between two other particles** (the scale `beta`), a pull toward the swarm's best (`social`) and a decaying inertia, and the new position is mixed with the old one by a crossover with probability `crossover_rate`. A particle that has not improved for `stagnation_limit` generations is re-seeded inside the range spanned by the swarm, which keeps the swarm from collapsing. It is chosen explicitly with `DfoSolverKind::ParticleSwarmDv`; `Auto` does not select it. On a heavily multimodal function such as the Rastrigin lattice it is less reliable than the classic swarm or the adaptive differential evolution, so run it from several seeds there; on a smooth valley it converges well.

## Parameters

`Fn &&objective`
: A callable `Series (Series)` (real scalar) or `Series -> number`.

`const pluss::Series &initial`
: The starting point.

`const dfo::ParticleSwarmDvConfig &config`
: Swarm size, differential and social scales, stagnation limit, budgets and bounds.

## Returns

A [`PopulationSolverOutcome`](PopulationSolverOutcome.md).

## Errors

- An objective that is not a real scalar throws.

## Practical Examples

### Example 1

A multimodal function. The Rastrigin function in a box.

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

  dfo::ParticleSwarmDvConfig config;
  config.lower_bounds = pluss::series::make({-5.12, -5.12, -5.12});
  config.upper_bounds = pluss::series::make({5.12, 5.12, 5.12});
  config.max_evaluations = 20000;
  const dfo::PopulationSolverOutcome o = dfo::particle_swarm_dv_minimizer(rastrigin, pluss::series::make({3.0, 3.0, 3.0}), config);
  out.writeln("minimizer =", o.minimizer.items<double>(), ", minimum =", o.minimum);
  out.writeln("status =", dfo::to_string(o.status), ", evaluations =", o.diagnostics.evaluations);
}
```

Output

```text
minimizer = [-0.032935, 0.091368, -1.970508] , minimum = 5.879991
status = ReachedEvaluationBudget , evaluations = 20130
```

### Example 2

A smooth valley: the Rosenbrock function in a box.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  dfo::ParticleSwarmDvConfig config;
  config.lower_bounds = pluss::series::make({-3.0, -3.0});
  config.upper_bounds = pluss::series::make({3.0, 3.0});
  config.population_size = 40;
  config.max_evaluations = 20000;
  const dfo::PopulationSolverOutcome o = dfo::particle_swarm_dv_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config);
  out.writeln("minimizer =", o.minimizer.items<double>(), ", minimum =", o.minimum);
}
```

Output

```text
minimizer = [0.999653, 0.999342] , minimum = 0.000000
```

### Example 3

Repeatability. The same seed gives the same run.

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

  for (const std::uint64_t seed : {5, 5, 6}) {
    dfo::ParticleSwarmDvConfig config;
    config.rng_seed = seed;
    config.lower_bounds = pluss::series::make({-5.12, -5.12, -5.12});
    config.upper_bounds = pluss::series::make({5.12, 5.12, 5.12});
    config.max_evaluations = 5000;
    out.writeln("seed", seed, ": minimum =", dfo::particle_swarm_dv_minimizer(rastrigin, pluss::series::make({3.0, 3.0, 3.0}), config).minimum);
  }
}
```

Output

```text
seed 5 : minimum = 4.65241494
seed 5 : minimum = 4.65241494
seed 6 : minimum = 11.52017175
```

## See Also

- [`ParticleSwarmDvConfig`](ParticleSwarmDvConfig.md): the settings
- [`particle_swarm_minimizer`](particle_swarm_minimizer.md): the classic swarm

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="orthomads_minimizer.html">
    <span class="label">Previous</span>
    <span class="title">orthomads_minimizer</span>
  </a>
  <a class="next-link" href="particle_swarm_minimizer.html">
    <span class="label">Next</span>
    <span class="title">particle_swarm_minimizer</span>
  </a>
</div>
<!-- apiref:nav:end -->
