<!-- generated from BlackboxSolverOutcome.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::BlackboxSolverOutcome`

The result of a scalar mesh-adaptive run.

`minimizer` and `minimum` are the best point and its value, `status` a [`BlackboxSolverStatus`](BlackboxSolverStatus.md) and `diagnostics` a [`BlackboxSolverDiagnostics`](BlackboxSolverDiagnostics.md).

## Fields

### `minimizer`

`pluss::Series`, default `undefined`.

The best point.

### `minimum`

`double`, default `0`.

The objective at the best point.

### `status`

`dfo::BlackboxSolverStatus`, default `ReachedIterationBudget`.

Why the run stopped.

### `diagnostics`

`dfo::BlackboxSolverDiagnostics`, default `{}`.

Counters.

## Guidance

Returned by the scalar mesh-adaptive minimizers.

## Practical Examples

### Example 1

Reading an outcome.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  const dfo::BlackboxSolverOutcome o = dfo::orthomads_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}));
  out.writeln("minimizer =", o.minimizer.items<double>(), ", minimum =", o.minimum, ", status =", dfo::to_string(o.status));
}
```

Output

```text
minimizer = [0.90775, 0.82350] , minimum = 0.00854 , status = ReachedEvaluationBudget
```

### Example 2

Comparing the three scalar variants.

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

  out.writeln("mesh_adaptive_direct_search :", dfo::mesh_adaptive_direct_search_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0})).minimum);
  out.writeln("orthomads                   :", dfo::orthomads_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0})).minimum);
  out.writeln("ltmads                      :", dfo::ltmads_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0})).minimum);
}
```

Output

```text
mesh_adaptive_direct_search : 0.00853561
orthomads                   : 0.00853561
ltmads                      : 0.00494886
```

### Example 3

The status of a short run.

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

  dfo::MadsBlackboxConfig config;
  config.max_iterations = 10;
  const dfo::BlackboxSolverOutcome o = dfo::orthomads_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config);
  out.writeln(dfo::to_string(o.status), ", iterations =", o.diagnostics.iterations);
}
```

Output

```text
ReachedIterationBudget , iterations = 10
```

## See Also

- [`orthomads_minimizer`](orthomads_minimizer.md): produces it
- [`BlackboxSolverDiagnostics`](BlackboxSolverDiagnostics.md): the counters

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="BlackboxSolverDisplay.html">
    <span class="label">Previous</span>
    <span class="title">BlackboxSolverDisplay</span>
  </a>
  <a class="next-link" href="BlackboxSolverStatus.html">
    <span class="label">Next</span>
    <span class="title">BlackboxSolverStatus</span>
  </a>
</div>
<!-- apiref:nav:end -->
