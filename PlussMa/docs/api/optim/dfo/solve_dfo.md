<!-- generated from solve_dfo.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::solve_dfo`

Minimises an objective from a starting point, choosing the method automatically.

## Signature

```cpp
template <class Fn> dfo::DfoResult dfo::solve_dfo(Fn &&objective, const pluss::Series &initial, const dfo::DfoOptions &options = {});
```

## What It Does

The shortest way in. `objective` takes a `Series` and returns either a scalar `Series` or an ordinary number; nothing is differentiated, so the function may contain `if`, `abs`, rounding or anything else that produces a value. `initial` is the starting point and fixes the number of variables. With the default options the front door picks a method from the problem (see [`DfoSolverKind`](DfoSolverKind.md)): for an unconstrained problem with a handful of variables that is Nelder-Mead. The result reports the minimizer, the minimum, how the run ended and which method ran.

To add bounds, constraints, a black box or a chosen method, fill in a [`DfoProblem`](DfoProblem.md) and call [`DFO::solve`](DFO/solve.md), or chain the setters of a [`DfoBuilder`](DfoBuilder.md); `solve_dfo` is a convenience for the objective-and-start case, and `DfoOptions` still applies.

## Parameters

`Fn &&objective`
: A callable `Series -> Series` (a real scalar) or `Series -> number`.

`const pluss::Series &initial`
: The starting point, a real floating-point `Series` with at least one element.

`const dfo::DfoOptions &options`
: Method choice, budgets, seed and per-method settings.

## Returns

A [`DfoResult`](DfoResult.md).

## Errors

- An undefined, empty or non-real `initial` throws.
- An objective that does not return a real scalar throws when it is first called.

## Practical Examples

### Example 1

A first solve. The minimum of `Σ (xᵢ − 3)²` is at `(3, 3)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x - 3.0).square().sum(); };   // minimum 0 at (3, 3)

  const dfo::DfoResult result = dfo::solve_dfo(f, pluss::series::make({0.0, 0.0}));
  out.writeln("minimizer =", result.minimizer.items<double>());
  out.writeln("minimum   =", result.minimum, ", status =", dfo::to_string(result.status), ", evaluations =", result.diagnostics.evaluations);
}
```

Output

```text
minimizer = [3.0000, 3.0000]
minimum   = 0.0000 , status = Converged , evaluations = 82
```

### Example 2

An objective that returns a number. The function is written with ordinary arithmetic on the elements; a `double` return is accepted.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) {
    const double a = x.select(0, 0).item<double>(), b = x.select(0, 1).item<double>();
    return (a - 1.0) * (a - 1.0) + 2.0 * (b + 2.0) * (b + 2.0);
  };
  const dfo::DfoResult result = dfo::solve_dfo(f, pluss::series::make({4.0, 4.0}));
  out.writeln("minimizer =", result.minimizer.items<double>(), ", minimum =", result.minimum);
}
```

Output

```text
minimizer = [1.0000, -2.0000] , minimum = 0.0000
```

### Example 3

A non-smooth objective. The sum of absolute values has a kink at its minimum, where a gradient method has no gradient; the simplex handles it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x - pluss::series::make({1.0, -1.0})).abs().sum(); };
  dfo::DfoOptions options;
  options.relative_improvement_tolerance = 1e-8;
  const dfo::DfoResult result = dfo::solve_dfo(f, pluss::series::make({3.0, 3.0}), options);
  out.writeln("minimizer =", result.minimizer.items<double>(), ", minimum =", result.minimum);
}
```

Output

```text
minimizer = [1.0000, -1.0000] , minimum = 0.0000
```

### Example 4

Choosing the method through the options.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto f = [](const Series &x) { return (x - 3.0).square().sum(); };   // minimum 0 at (3, 3)

  dfo::DfoOptions options;
  options.solver = dfo::DfoSolverKind::PatternSearch;
  const dfo::DfoResult result = dfo::solve_dfo(f, pluss::series::make({0.0, 0.0}), options);
  out.writeln(dfo::to_string(result.selected_solver), "| minimum =", result.minimum);
}
```

Output

```text
PatternSearch | minimum = 0.000000
```

## See Also

- [`DFO::solve`](DFO/solve.md): the same run from a problem value
- [`DfoBuilder`](DfoBuilder.md): chained setters

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="psdmads_blackbox_minimizer.html">
    <span class="label">Previous</span>
    <span class="title">psdmads_blackbox_minimizer</span>
  </a>
  <a class="next-link" href="to_string.html">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>
<!-- apiref:nav:end -->
