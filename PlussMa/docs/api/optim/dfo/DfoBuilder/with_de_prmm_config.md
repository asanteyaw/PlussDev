<!-- generated from with_de_prmm_config.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 12ab3fbcb8a7 -->

# `DfoBuilder::with_de_prmm_config`

[Back to `dfo::DfoBuilder`](../DfoBuilder.md)

Sets the settings of the adaptive differential evolution method.

## Signature

```cpp
dfo::DfoBuilder dfo::DfoBuilder::with_de_prmm_config(const dfo::DifferentialEvolutionPrmmConfig &cfg) const;
```

## What It Does

Sets `DfoOptions::differential_evolution_prmm`, a [`DifferentialEvolutionPrmmConfig`](../DifferentialEvolutionPrmmConfig.md) that carries every setting of the adaptive differential evolution method. It is read only when that method runs, whether chosen by `Auto` or by [`with_solver`](with_solver.md). The common settings (budgets, tolerance, seed, display) and the bounds of the problem are applied on top of it when the method starts.

## Parameters

`const dfo::DifferentialEvolutionPrmmConfig &cfg`
: The adaptive differential evolution settings.

## Returns

A new builder with the change; the original is untouched.

## Practical Examples

### Example 1

The elite size.

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

  for (const std::int64_t k : {2, 10}) {
    dfo::DifferentialEvolutionPrmmConfig cfg;
    cfg.elite_count = k;
    const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0, 0.0, 0.0})).with_objective(f).with_solver(dfo::DfoSolverKind::DifferentialEvolutionPrmm).with_de_prmm_config(cfg).with_max_evaluations(5000).solve();
    out.writeln("elites", k, ": evaluations =", r.diagnostics.evaluations, ", minimum =", r.minimum);
  }
}
```

Output

```text
elites 2 : evaluations = 5000 , minimum = 0.00000000
elites 10 : evaluations = 5000 , minimum = 0.00000000
```

### Example 2

A smaller population.

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

  dfo::DifferentialEvolutionPrmmConfig cfg;
  cfg.population_size = 40;
  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0, 0.0, 0.0})).with_objective(f).with_solver(dfo::DfoSolverKind::DifferentialEvolutionPrmm).with_de_prmm_config(cfg).with_max_evaluations(5000).solve();
  out.writeln("evaluations =", r.diagnostics.evaluations, ", minimum =", r.minimum);
}
```

Output

```text
evaluations = 3410 , minimum = 0.00000000
```

### Example 3

The reduction passes.

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

  dfo::DifferentialEvolutionPrmmConfig cfg;
  cfg.reduction_passes = 1;
  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0, 0.0, 0.0})).with_objective(f).with_solver(dfo::DfoSolverKind::DifferentialEvolutionPrmm).with_de_prmm_config(cfg).with_max_evaluations(5000).solve();
  out.writeln("evaluations =", r.diagnostics.evaluations, ", minimum =", r.minimum);
}
```

Output

```text
evaluations = 2800 , minimum = 0.00349939
```

## See Also

- [`DfoBuilder`](../DfoBuilder.md): the class

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="with_de_config.html">
    <span class="label">Previous</span>
    <span class="title">with_de_config</span>
  </a>
  <a class="next-link" href="with_display_iter.html">
    <span class="label">Next</span>
    <span class="title">with_display_iter</span>
  </a>
</div>
<!-- apiref:nav:end -->
