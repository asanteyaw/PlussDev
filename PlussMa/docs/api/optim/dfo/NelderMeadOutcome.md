<!-- generated from NelderMeadOutcome.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::NelderMeadOutcome`

The result of a Nelder-Mead run: the minimizer, the minimum, the status and the counters.

`minimizer` is the best vertex of the final simplex and `minimum` its objective value. `status` is a [`SolverStatus`](SolverStatus.md) and `diagnostics` holds the iteration and evaluation counts and the name of the last step.

## Fields

### `minimizer`

`pluss::Series`, default `undefined`.

The best point.

### `minimum`

`double`, default `0`.

The objective at the best point.

### `status`

`dfo::SolverStatus`, default `ReachedIterationBudget`.

Why the run stopped.

### `diagnostics`

`dfo::NelderMeadDiagnostics`, default `{}`.

Counters.

## Guidance

Returned by `nelder_mead_minimizer`.

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
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  const dfo::NelderMeadOutcome o = dfo::nelder_mead_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}));
  out.writeln("minimizer =", o.minimizer.items<double>());
  out.writeln("minimum =", o.minimum, ", status =", dfo::to_string(o.status));
}
```

Output

```text
minimizer = [1.000022, 1.000042]
minimum = 0.000000 , status = Converged
```

### Example 2

Judging a result by its status.

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

  dfo::NelderMeadConfig config;
  config.max_evaluations = 40;
  const dfo::NelderMeadOutcome o = dfo::nelder_mead_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config);
  out.writeln("converged =", o.status == dfo::SolverStatus::Converged, ", minimum =", o.minimum);
}
```

Output

```text
converged = false , minimum = 2.0022
```

### Example 3

The diagnostics.

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

  const dfo::NelderMeadOutcome o = dfo::nelder_mead_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}));
  out.writeln("iterations =", o.diagnostics.iterations, ", evaluations =", o.diagnostics.evaluations, ", last step =", o.diagnostics.last_step);
}
```

Output

```text
iterations = 85 , evaluations = 159 , last step = adjust_inner
```

## See Also

- [`nelder_mead_minimizer`](nelder_mead_minimizer.md): produces it
- [`NelderMeadDiagnostics`](NelderMeadDiagnostics.md): the counters

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="NelderMeadDiagnostics.html">
    <span class="label">Previous</span>
    <span class="title">NelderMeadDiagnostics</span>
  </a>
  <a class="next-link" href="PSBasisFamily.html">
    <span class="label">Next</span>
    <span class="title">PSBasisFamily</span>
  </a>
</div>
<!-- apiref:nav:end -->
