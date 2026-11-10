<!-- generated from to_string_PopulationSolverStatus.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::to_string(PopulationSolverStatus)`

Returns the name of a population-method status.

## Signature

```cpp
std::string dfo::to_string(dfo::PopulationSolverStatus status);
```

## What It Does

Returns the enumerator name as text: `"Converged"`, `"ReachedIterationBudget"` or `"ReachedEvaluationBudget"`.

## Parameters

`dfo::PopulationSolverStatus status`
: The status to name.

## Returns

The enumerator name.

## Practical Examples

### Example 1

Naming values.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  out.writeln(dfo::to_string(dfo::PopulationSolverStatus::Converged), dfo::to_string(dfo::PopulationSolverStatus::ReachedIterationBudget), dfo::to_string(dfo::PopulationSolverStatus::ReachedEvaluationBudget));
}
```

Output

```text
Converged ReachedIterationBudget ReachedEvaluationBudget
```

### Example 2

Naming a result's status.

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
  config.max_generations = 3;
  config.initial_radius = 5.0;
  out.writeln("status =", dfo::to_string(dfo::particle_swarm_minimizer(f, pluss::series::make({0.0, 0.0, 0.0, 0.0}), config).status));
}
```

Output

```text
status = ReachedIterationBudget
```

### Example 3

In a table of budgets.

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

  for (const std::int64_t n : {300, 100000}) {
    dfo::DifferentialEvolutionConfig config;
    config.max_evaluations = n;
    config.check_every = 5;
    config.initial_radius = 5.0;
    out.writeln("budget", n, ":", dfo::to_string(dfo::differential_evolution_minimizer(f, pluss::series::make({0.0, 0.0, 0.0, 0.0}), config).status));
  }
}
```

Output

```text
budget 300 : ReachedEvaluationBudget
budget 100000 : Converged
```

## See Also

- [`PopulationSolverStatus`](PopulationSolverStatus.md): the values

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_string_PSStatus.html">
    <span class="label">Previous</span>
    <span class="title">to_string(PSStatus)</span>
  </a>
  <a class="next-link" href="to_string_SolverStatus.html">
    <span class="label">Next</span>
    <span class="title">to_string(SolverStatus)</span>
  </a>
</div>
<!-- apiref:nav:end -->
