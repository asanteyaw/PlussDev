<!-- generated from nelder_mead_minimizer.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::nelder_mead_minimizer`

Minimises an objective with the Nelder-Mead simplex method.

## Signature

```cpp
dfo::NelderMeadOutcome dfo::nelder_mead_minimizer(const std::function<pluss::Series(const pluss::Series &)> &objective, const pluss::Series &initial, const dfo::NelderMeadConfig &config = {});
template <class Fn> dfo::NelderMeadOutcome dfo::nelder_mead_minimizer(Fn &&objective, const pluss::Series &initial, const dfo::NelderMeadConfig &config = {});
```

## What It Does

The simplex method keeps `n + 1` points in `n` dimensions and repeatedly replaces the worst one by reflecting it through the centre of the others, expanding the step when that works well and contracting or shrinking the whole simplex when it does not. It uses objective values only, so the function may be non-smooth, and it needs no bounds or gradient. It is the method [`Auto`](DfoSolverKind.md) chooses for small unconstrained problems.

The objective may return a real scalar `Series` or an ordinary number. An objective may return `+infinity` to say "this point is not allowed": the simplex treats it as a very bad value and moves away, which keeps a search inside a region that is easy to test but awkward to write as a bound. A NaN or `−infinity` cannot be ordered and is an error. Starting from `initial`, the first simplex is built by moving each coordinate by the relative amount `simplex_delta` (or by `simplex_delta_zero` for a coordinate that is zero).

The result is a [`NelderMeadOutcome`](NelderMeadOutcome.md). Nelder-Mead has no built-in bounds or constraints: the front door adds bounds by projection, and a forbidden region can be expressed with an infinite value.

## Parameters

`Fn &&objective`
: A callable `Series -> Series` (real scalar) or `Series -> number`; may return `+infinity` for a forbidden point.

`const pluss::Series &initial`
: The starting point.

`const dfo::NelderMeadConfig &config`
: Tolerances, budgets and step coefficients.

## Returns

A [`NelderMeadOutcome`](NelderMeadOutcome.md).

## Errors

- An objective that is not a real scalar throws.
- A NaN or `−infinity` value throws.

## Practical Examples

### Example 1

The Rosenbrock function from the classic start. Its minimum is 0 at `(1, 1)`.

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
  out.writeln("minimizer =", o.minimizer.items<double>(), ", minimum =", o.minimum);
  out.writeln("status =", dfo::to_string(o.status), ", evaluations =", o.diagnostics.evaluations);
}
```

Output

```text
minimizer = [1.000022, 1.000042] , minimum = 0.000000
status = Converged , evaluations = 159
```

### Example 2

A forbidden region. The objective returns infinity outside the disc `x₀² + x₁² ≤ 4`; the exact answer is the nearest point of the disc to `(3, 3)`, `(√2, √2)` with value 5.0294. The simplex gets close to it, within the default tolerances.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <limits>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) -> double {
    const double a = x.select(0, 0).item<double>(), b = x.select(0, 1).item<double>();
    if (a * a + b * b > 4.0) return std::numeric_limits<double>::infinity();
    return (a - 3.0) * (a - 3.0) + (b - 3.0) * (b - 3.0);
  };
  const dfo::NelderMeadOutcome o = dfo::nelder_mead_minimizer(f, pluss::series::make({0.0, 0.0}));
  out.writeln("minimizer =", o.minimizer.items<double>(), ", minimum =", o.minimum, " (exact 5.0294)");
}
```

Output

```text
minimizer = [1.3744, 1.4529] , minimum = 5.0360  (exact 5.0294)
```

### Example 3

A configuration. A budget of 60 evaluations and a larger first simplex.

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
  config.max_evaluations = 60;
  config.simplex_delta = 0.5;
  const dfo::NelderMeadOutcome o = dfo::nelder_mead_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config);
  out.writeln(dfo::to_string(o.status), ", evaluations =", o.diagnostics.evaluations, ", minimum =", o.minimum);
}
```

Output

```text
ReachedEvaluationBudget , evaluations = 60 , minimum = 1.3684
```

### Example 4

Higher dimension. The same call minimises a six-variable quadratic. The simplex loses accuracy as the number of variables grows (the minimum reached here is about 2e-3, not machine precision); for many variables prefer pattern search or a population method.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto f = [](const Series &x) { return (x - 1.0).square().sum(); };
  const dfo::NelderMeadOutcome o = dfo::nelder_mead_minimizer(f, pluss::series::make({0.0, 0.0, 0.0, 0.0, 0.0, 0.0}));
  out.writeln("minimum =", o.minimum, ", evaluations =", o.diagnostics.evaluations, ", status =", dfo::to_string(o.status));
}
```

Output

```text
minimum = 0.002057 , evaluations = 930 , status = Converged
```

## See Also

- [`NelderMeadConfig`](NelderMeadConfig.md): the settings
- [`DfoSolverKind`](DfoSolverKind.md): the front door's `NelderMead`

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mesh_adaptive_direct_search_minimizer.html">
    <span class="label">Previous</span>
    <span class="title">mesh_adaptive_direct_search_minimizer</span>
  </a>
  <a class="next-link" href="orthomads_blackbox_minimizer.html">
    <span class="label">Next</span>
    <span class="title">orthomads_blackbox_minimizer</span>
  </a>
</div>
<!-- apiref:nav:end -->
