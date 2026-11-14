<!-- generated from particle_swarm_minimizer.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::particle_swarm_minimizer`

Minimises an objective with a particle swarm: points that move with inertia toward their own best and the swarm's best.

## Signature

```cpp
dfo::PopulationSolverOutcome dfo::particle_swarm_minimizer(const std::function<pluss::Series(const pluss::Series &)> &objective, const pluss::Series &initial, const dfo::ParticleSwarmConfig &config = {});
template <class Fn> dfo::PopulationSolverOutcome dfo::particle_swarm_minimizer(Fn &&objective, const pluss::Series &initial, const dfo::ParticleSwarmConfig &config = {});
```

## What It Does

Each particle has a position and a velocity. Every generation the velocity is updated by three pulls: **inertia** (keep moving), the **cognitive** pull toward the particle's own best position and the **social** pull toward the best position any particle has found, each scaled by random numbers; the particle then moves. By default the inertia falls linearly from `inertia_max` to `inertia_min` over the run, so the swarm explores first and settles later, and a particle sits at the centre of the box as an extra start (`center_particle`). Positions stay inside the bounds.

Swarms are cheap per particle and scale well to many variables, which is why [`Auto`](DfoSolverKind.md) uses them from 40 variables. The sampling region is the box when bounds are given, otherwise `initial ± initial_radius`.

## Parameters

`Fn &&objective`
: A callable `Series (Series)` (real scalar) or `Series -> number`.

`const pluss::Series &initial`
: The starting point.

`const dfo::ParticleSwarmConfig &config`
: Swarm size, inertia and acceleration schedules, budgets and bounds.

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

  dfo::ParticleSwarmConfig config;
  config.lower_bounds = pluss::series::make({-5.12, -5.12, -5.12});
  config.upper_bounds = pluss::series::make({5.12, 5.12, 5.12});
  config.max_evaluations = 20000;
  const dfo::PopulationSolverOutcome o = dfo::particle_swarm_minimizer(rastrigin, pluss::series::make({3.0, 3.0, 3.0}), config);
  out.writeln("minimizer =", o.minimizer.items<double>(), ", minimum =", o.minimum);
  out.writeln("status =", dfo::to_string(o.status), ", generations =", o.diagnostics.iterations, ", evaluations =", o.diagnostics.evaluations);
}
```

Output

```text
minimizer = [-0.000000, 0.000000, 0.000000] , minimum = 0.000000
status = ReachedEvaluationBudget , generations = 99 , evaluations = 20099
```

### Example 2

Many variables. Twenty coordinates of a shifted quadratic.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto f = [](const Series &x) { return (x - 2.0).square().sum(); };
  dfo::ParticleSwarmConfig config;
  config.initial_radius = 6.0;
  config.population_size = 60;
  config.max_evaluations = 60000;
  const dfo::PopulationSolverOutcome o = dfo::particle_swarm_minimizer(f, pluss::series::make(std::vector<double>(20, 0.0)), config);
  out.writeln("minimum =", o.minimum, ", evaluations =", o.diagnostics.evaluations);
}
```

Output

```text
minimum = 0.00000000 , evaluations = 60023
```

### Example 3

A budget.

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
  config.max_evaluations = 1000;
  const dfo::PopulationSolverOutcome o = dfo::particle_swarm_minimizer(rastrigin, pluss::series::make({3.0, 3.0, 3.0}), config);
  out.writeln(dfo::to_string(o.status), ", evaluations =", o.diagnostics.evaluations, ", minimum =", o.minimum);
}
```

Output

```text
ReachedEvaluationBudget , evaluations = 1004 , minimum = 1.2850
```

## See Also

- [`ParticleSwarmConfig`](ParticleSwarmConfig.md): the settings
- [`particle_swarm_dv_minimizer`](particle_swarm_dv_minimizer.md): the differential-velocity variant

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="particle_swarm_dv_minimizer.html">
    <span class="label">Previous</span>
    <span class="title">particle_swarm_dv_minimizer</span>
  </a>
  <a class="next-link" href="pattern_search_minimizer.html">
    <span class="label">Next</span>
    <span class="title">pattern_search_minimizer</span>
  </a>
</div>
<!-- apiref:nav:end -->
