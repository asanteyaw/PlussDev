<!-- generated from BlackboxSolverStatus.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::BlackboxSolverStatus`

Why a mesh-adaptive run stopped.

`Converged` means the frame size fell to `mesh_tolerance` (or the relative improvement test was met). `ReachedIterationBudget` and `ReachedEvaluationBudget` mean `max_iterations` or `max_evaluations` ran out first. Converged says the mesh could not be improved at its finest resolution; it is not a proof of a minimum.

## Fields

### `Converged`

`enumerator`, default `0`.

The frame fell to `mesh_tolerance`.

### `ReachedIterationBudget`

`enumerator`, default `1`.

`max_iterations` ran out.

### `ReachedEvaluationBudget`

`enumerator`, default `2`.

`max_evaluations` ran out.

## Guidance

Read from the `status` of a mesh-adaptive outcome.

## Practical Examples

### Example 1

Each outcome.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  dfo::MadsBlackboxConfig by_iterations, by_evaluations;
  by_iterations.max_iterations = 10;
  by_evaluations.max_evaluations = 50;
  out.writeln("default       :", dfo::to_string(dfo::orthomads_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0})).status));
  out.writeln("10 iterations :", dfo::to_string(dfo::orthomads_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), by_iterations).status));
  out.writeln("50 evaluations:", dfo::to_string(dfo::orthomads_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), by_evaluations).status));
}
```

Output

```text
default       : ReachedEvaluationBudget
10 iterations : ReachedIterationBudget
50 evaluations: ReachedEvaluationBudget
```

### Example 2

The black-box form.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto black_box = [](const Series &x) {
    dfo::BlackboxEvaluation e;
    e.objectives = (x - 3.0).square().sum().reshape({1});   // minimum 0 at (3, 3)
    return e;
  };

  dfo::MadsBlackboxAdvancedConfig config;
  config.max_evaluations = 30;
  out.writeln(dfo::to_string(dfo::mads_blackbox_minimizer(black_box, pluss::series::make({0.0, 0.0}), config).status));
}
```

Output

```text
ReachedEvaluationBudget
```

### Example 3

Acting on the status.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  const dfo::BlackboxSolverOutcome o = dfo::orthomads_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}));
  out.writeln(o.status == dfo::BlackboxSolverStatus::Converged ? "the mesh converged" : "a budget ran out");
}
```

Output

```text
a budget ran out
```

## See Also

- [`BlackboxSolverOutcome`](BlackboxSolverOutcome.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="BlackboxSolverOutcome.html">
    <span class="label">Previous</span>
    <span class="title">BlackboxSolverOutcome</span>
  </a>
  <a class="next-link" href="ConstraintHandling.html">
    <span class="label">Next</span>
    <span class="title">ConstraintHandling</span>
  </a>
</div>
<!-- apiref:nav:end -->
