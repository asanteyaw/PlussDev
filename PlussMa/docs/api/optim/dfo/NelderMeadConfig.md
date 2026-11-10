<!-- generated from NelderMeadConfig.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::NelderMeadConfig`

Tolerances, budgets and step coefficients of the Nelder-Mead method.

The run stops as `Converged` when the simplex is smaller than `position_tolerance` and its values agree to `value_tolerance`, or earlier when `max_iterations` or `max_evaluations` run out; zero keeps the default limit of 200 times the number of variables for each. The four coefficients are the standard ones: `reflection` 1, `expansion` 2, `contraction` 0.5, `shrink` 0.5; changing them trades speed against robustness. `simplex_delta` is the relative size of the starting simplex and `simplex_delta_zero` its absolute size for a coordinate that starts at zero.

## Fields

### `position_tolerance`

`double`, default `1e-4`.

Converged when the simplex's size is below this (together with `value_tolerance`).

### `value_tolerance`

`double`, default `1e-4`.

Converged when the objective values at the vertices agree to this.

### `max_iterations`

`std::int64_t`, default `0`.

Iteration limit; 0 uses 200 × the number of variables.

### `max_evaluations`

`std::int64_t`, default `0`.

Evaluation limit; 0 uses 200 × the number of variables.

### `reflection`

`double`, default `1`.

Reflection coefficient.

### `expansion`

`double`, default `2`.

Expansion coefficient.

### `contraction`

`double`, default `0.5`.

Contraction coefficient.

### `shrink`

`double`, default `0.5`.

Shrink coefficient.

### `simplex_delta`

`double`, default `0.05`.

Relative size of the starting simplex for a nonzero coordinate.

### `simplex_delta_zero`

`double`, default `0.00025`.

Absolute size of the starting simplex for a zero coordinate.

### `promote_cpu_to_float64`

`bool`, default `true`.

Compute single-precision CPU inputs in double precision.

### `display`

`dfo::SolverDisplay`, default `None`.

Print a line per iteration when `Iter`.

## Guidance

Passed to `nelder_mead_minimizer`, or set as `DfoOptions::nelder_mead`.

## Practical Examples

### Example 1

Tolerances. Tighter tolerances cost evaluations and buy accuracy.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  for (const double tol : {1e-2, 1e-6, 1e-10}) {
    dfo::NelderMeadConfig config;
    config.position_tolerance = tol;
    config.value_tolerance = tol;
    const dfo::NelderMeadOutcome o = dfo::nelder_mead_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config);
    out.writeln("tolerance", tol, ": evaluations =", o.diagnostics.evaluations, ", minimum =", o.minimum);
  }
}
```

Output

```text
tolerance 0.0100000000 : evaluations = 130 , minimum = 0.0000084608
tolerance 0.0000010000 : evaluations = 189 , minimum = 0.0000000000
tolerance 0.0000000001 : evaluations = 249 , minimum = 0.0000000000
```

### Example 2

The starting simplex. A small simplex explores locally and a large one covers more ground.

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

  for (const double delta : {0.001, 0.05, 0.5}) {
    dfo::NelderMeadConfig config;
    config.simplex_delta = delta;
    const dfo::NelderMeadOutcome o = dfo::nelder_mead_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config);
    out.writeln("simplex_delta", delta, ": evaluations =", o.diagnostics.evaluations, ", minimum =", o.minimum);
  }
}
```

Output

```text
simplex_delta 0.00100000 : evaluations = 228 , minimum = 0.00000000
simplex_delta 0.05000000 : evaluations = 159 , minimum = 0.00000000
simplex_delta 0.50000000 : evaluations = 190 , minimum = 0.00000000
```

### Example 3

The step coefficients. A bolder expansion and a timid contraction, against the defaults.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <utility>

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

  dfo::NelderMeadConfig bold, timid;
  bold.expansion = 3.0;
  timid.contraction = 0.25;
  timid.shrink = 0.25;
  for (const auto &[label, config] : {std::pair{"defaults", dfo::NelderMeadConfig{}}, std::pair{"expansion 3", bold}, std::pair{"contraction 0.25", timid}}) {
    const dfo::NelderMeadOutcome o = dfo::nelder_mead_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config);
    out.writeln(label, ": evaluations =", o.diagnostics.evaluations, ", minimum =", o.minimum);
  }
}
```

Output

```text
defaults : evaluations = 159 , minimum = 0.00000000
expansion 3 : evaluations = 184 , minimum = 0.00000000
contraction 0.25 : evaluations = 173 , minimum = 0.00000000
```

### Example 4

A budget. The run ends `ReachedEvaluationBudget` when the evaluations run out.

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

  dfo::NelderMeadConfig config;
  config.max_evaluations = 100;
  const dfo::NelderMeadOutcome o = dfo::nelder_mead_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config);
  out.writeln(dfo::to_string(o.status), ", evaluations =", o.diagnostics.evaluations, ", minimum =", o.minimum);
}
```

Output

```text
ReachedEvaluationBudget , evaluations = 100 , minimum = 0.056929
```

## See Also

- [`nelder_mead_minimizer`](nelder_mead_minimizer.md): uses it
- [`DfoOptions`](DfoOptions.md): holds one

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="MultiObjectiveScalarization.html">
    <span class="label">Previous</span>
    <span class="title">MultiObjectiveScalarization</span>
  </a>
  <a class="next-link" href="NelderMeadDiagnostics.html">
    <span class="label">Next</span>
    <span class="title">NelderMeadDiagnostics</span>
  </a>
</div>
<!-- apiref:nav:end -->
