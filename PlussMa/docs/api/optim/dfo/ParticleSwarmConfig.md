<!-- generated from ParticleSwarmConfig.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::ParticleSwarmConfig`

Swarm size, inertia and acceleration schedules, velocity clamp and stopping settings of the particle swarm.

With `linear_inertia_schedule` on, the inertia falls linearly from `inertia_max` to `inertia_min`; with it off it starts at `inertia` and is multiplied by `inertia_damp` each generation. `cognitive` and `social` are the pull coefficients; with `time_varying_acceleration` they move to `final_cognitive` and `final_social` over the run, shifting from individual to collective search. `velocity_clamp_ratio` limits the speed to that fraction of the box width (0 for no limit), and `center_particle` adds the box centre as an extra member.

## Fields

### `population_size`

`std::int64_t`, default `200`.

Particles.

### `max_generations`

`std::int64_t`, default `1000`.

Generation limit.

### `max_evaluations`

`std::int64_t`, default `0`.

Evaluation limit; 0 leaves the generation limit as the only bound.

### `check_every`

`std::int64_t`, default `0`.

Generations between improvement tests; 0 disables the test.

### `relative_improvement_tolerance`

`double`, default `1e-8`.

Stop when the relative improvement is below this.

### `center_particle`

`bool`, default `true`.

Add the centre of the box as an extra particle.

### `linear_inertia_schedule`

`bool`, default `true`.

Decay the inertia linearly from `inertia_max` to `inertia_min`.

### `time_varying_acceleration`

`bool`, default `false`.

Move `cognitive`/`social` to `final_cognitive`/`final_social` over the run.

### `inertia`

`double`, default `1`.

Starting inertia when the schedule is off.

### `inertia_max`

`double`, default `0.99`.

Inertia at the start of the linear schedule.

### `inertia_min`

`double`, default `0.1`.

Inertia at the end of the linear schedule.

### `inertia_damp`

`double`, default `0.99`.

Per-generation inertia factor when the schedule is off.

### `cognitive`

`double`, default `2`.

Pull toward the particle's own best.

### `social`

`double`, default `2`.

Pull toward the swarm's best.

### `final_cognitive`

`double`, default `0.5`.

Final cognitive pull with a time-varying schedule.

### `final_social`

`double`, default `2.5`.

Final social pull with a time-varying schedule.

### `velocity_clamp_ratio`

`double`, default `0`.

Largest speed as a fraction of the box width; 0 for none.

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

Passed to `particle_swarm_minimizer`, or set as `DfoOptions::particle_swarm`.

## Practical Examples

### Example 1

The swarm size.

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

  for (const std::int64_t n : {20, 60, 200}) {
    dfo::ParticleSwarmConfig config;
    config.population_size = n;
    config.lower_bounds = pluss::series::make({-5.12, -5.12, -5.12});
    config.upper_bounds = pluss::series::make({5.12, 5.12, 5.12});
    config.max_evaluations = 20000;
    const dfo::PopulationSolverOutcome o = dfo::particle_swarm_minimizer(rastrigin, pluss::series::make({3.0, 3.0, 3.0}), config);
    out.writeln("swarm", n, ": evaluations =", o.diagnostics.evaluations, ", minimum =", o.minimum);
  }
}
```

Output

```text
swarm 20 : evaluations = 20012 , minimum = 0.000000
swarm 60 : evaluations = 20007 , minimum = 0.000000
swarm 200 : evaluations = 20099 , minimum = 0.000000
```

### Example 2

The pulls. A strong social pull moves every particle toward the swarm's best; a strong cognitive pull keeps particles tied to their own best. Here the cognitive-heavy swarm ends highest.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <tuple>

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

  for (const auto &[label, cognitive, social] : {std::tuple{"balanced 2/2", 2.0, 2.0}, std::tuple{"social 0.5/3", 0.5, 3.0}, std::tuple{"cognitive 3/0.5", 3.0, 0.5}}) {
    dfo::ParticleSwarmConfig config;
    config.cognitive = cognitive;
    config.social = social;
    config.lower_bounds = pluss::series::make({-5.12, -5.12, -5.12});
    config.upper_bounds = pluss::series::make({5.12, 5.12, 5.12});
    config.max_evaluations = 10000;
    out.writeln(label, ": minimum =", dfo::particle_swarm_minimizer(rastrigin, pluss::series::make({3.0, 3.0, 3.0}), config).minimum);
  }
}
```

Output

```text
balanced 2/2 : minimum = 0.000000
social 0.5/3 : minimum = 0.000000
cognitive 3/0.5 : minimum = 0.002061
```

### Example 3

The inertia schedule: linear decay against a fixed damping.

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

  for (const bool linear : {true, false}) {
    dfo::ParticleSwarmConfig config;
    config.linear_inertia_schedule = linear;
    config.lower_bounds = pluss::series::make({-5.12, -5.12, -5.12});
    config.upper_bounds = pluss::series::make({5.12, 5.12, 5.12});
    config.max_evaluations = 10000;
    const dfo::PopulationSolverOutcome o = dfo::particle_swarm_minimizer(rastrigin, pluss::series::make({3.0, 3.0, 3.0}), config);
    out.writeln(linear ? "linear decay " : "damped      ", ": generations =", o.diagnostics.iterations, ", minimum =", o.minimum);
  }
}
```

Output

```text
linear decay  : generations = 49 , minimum = 0.000000
damped       : generations = 49 , minimum = 1.594779
```

### Example 4

A velocity clamp keeps particles from overshooting the box.

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

  for (const double clamp : {0.0, 0.2}) {
    dfo::ParticleSwarmConfig config;
    config.velocity_clamp_ratio = clamp;
    config.lower_bounds = pluss::series::make({-5.12, -5.12, -5.12});
    config.upper_bounds = pluss::series::make({5.12, 5.12, 5.12});
    config.max_evaluations = 10000;
    out.writeln("clamp", clamp, ": minimum =", dfo::particle_swarm_minimizer(rastrigin, pluss::series::make({3.0, 3.0, 3.0}), config).minimum);
  }
}
```

Output

```text
clamp 0.000000 : minimum = 0.000000
clamp 0.200000 : minimum = 0.000000
```

## See Also

- [`particle_swarm_minimizer`](particle_swarm_minimizer.md): uses it
- [`DfoOptions`](DfoOptions.md): holds one

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="PSStatus.html">
    <span class="label">Previous</span>
    <span class="title">PSStatus</span>
  </a>
  <a class="next-link" href="ParticleSwarmDvConfig.html">
    <span class="label">Next</span>
    <span class="title">ParticleSwarmDvConfig</span>
  </a>
</div>
<!-- apiref:nav:end -->
