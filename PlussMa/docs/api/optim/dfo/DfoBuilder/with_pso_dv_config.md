<!-- generated from with_pso_dv_config.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e469748f5da4 -->

# `DfoBuilder::with_pso_dv_config`

[Back to `dfo::DfoBuilder`](../DfoBuilder.md)

Sets the settings of the differential-velocity swarm method.

## Signature

```cpp
dfo::DfoBuilder dfo::DfoBuilder::with_pso_dv_config(const dfo::ParticleSwarmDvConfig &cfg) const;
```

## What It Does

Sets `DfoOptions::particle_swarm_dv`, a [`ParticleSwarmDvConfig`](../ParticleSwarmDvConfig.md) that carries every setting of the differential-velocity swarm method. It is read only when that method runs, whether chosen by `Auto` or by [`with_solver`](with_solver.md). The common settings (budgets, tolerance, seed, display) and the bounds of the problem are applied on top of it when the method starts.

## Parameters

`const dfo::ParticleSwarmDvConfig &cfg`
: The differential-velocity swarm settings.

## Returns

A new builder with the change; the original is untouched.

## Practical Examples

### Example 1

The crossover rate.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto f = [](const Series &x) { return (x - pluss::series::make({1.0, -2.0, 0.5, 3.0})).square().sum(); };   // minimum 0 at (1, -2, 0.5, 3)

  for (const double cr : {0.3, 0.9}) {
    dfo::ParticleSwarmDvConfig cfg;
    cfg.crossover_rate = cr;
    const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0, 0.0, 0.0})).with_objective(f).with_solver(dfo::DfoSolverKind::ParticleSwarmDv).with_pso_dv_config(cfg).solve();
    out.writeln("crossover", cr, ": evaluations =", r.diagnostics.evaluations, ", minimum =", r.minimum);
  }
}
```

Output

```text
crossover 0.30000000 : evaluations = 50313 , minimum = 0.00000000
crossover 0.90000000 : evaluations = 44241 , minimum = 0.00000000
```

### Example 2

The stagnation limit: how many generations a particle may go without improving before it is re-seeded. A larger limit gives a better answer on the Rosenbrock function with the same generation budget.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  for (const std::int64_t limit : {10, 50, 200}) {
    dfo::ParticleSwarmDvConfig cfg;
    cfg.stagnation_limit = limit;
    cfg.population_size = 40;
    const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(rosenbrock).with_solver(dfo::DfoSolverKind::ParticleSwarmDv).with_pso_dv_config(cfg).with_max_iterations(300).solve();
    out.writeln("stagnation limit", limit, ": minimum =", r.minimum);
  }
}
```

Output

```text
stagnation limit 10 : minimum = 0.00013966
stagnation limit 50 : minimum = 0.00000204
stagnation limit 200 : minimum = 0.00000001
```

### Example 3

The swarm size.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto f = [](const Series &x) { return (x - pluss::series::make({1.0, -2.0, 0.5, 3.0})).square().sum(); };   // minimum 0 at (1, -2, 0.5, 3)

  dfo::ParticleSwarmDvConfig cfg;
  cfg.population_size = 30;
  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0, 0.0, 0.0})).with_objective(f).with_solver(dfo::DfoSolverKind::ParticleSwarmDv).with_pso_dv_config(cfg).solve();
  out.writeln("evaluations =", r.diagnostics.evaluations, ", minimum =", r.minimum);
}
```

Output

```text
evaluations = 8131 , minimum = 0.00000000
```

## See Also

- [`DfoBuilder`](../DfoBuilder.md): the class

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="with_pso_config.html">
    <span class="label">Previous</span>
    <span class="title">with_pso_config</span>
  </a>
  <a class="next-link" href="with_seed.html">
    <span class="label">Next</span>
    <span class="title">with_seed</span>
  </a>
</div>
<!-- apiref:nav:end -->
