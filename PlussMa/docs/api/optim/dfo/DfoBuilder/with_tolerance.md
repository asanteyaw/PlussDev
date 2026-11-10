<!-- generated from with_tolerance.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 616f5ad66c62 -->

# `DfoBuilder::with_tolerance`

[Back to `dfo::DfoBuilder`](../DfoBuilder.md)

Sets the relative-improvement stopping tolerance.

## Signature

```cpp
dfo::DfoBuilder dfo::DfoBuilder::with_tolerance(double tolerance) const;
```

## What It Does

Sets `DfoOptions::relative_improvement_tolerance`. When positive it replaces the method's own stopping tolerance (Nelder-Mead's position and value tolerances, the relative improvement of the population and mesh methods). A smaller tolerance asks for a more accurate answer at the cost of more evaluations.

## Parameters

`double tolerance`
: The tolerance; 0 keeps each method's default.

## Returns

A new builder with the change; the original is untouched.

## Practical Examples

### Example 1

Loose and tight.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(12));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  for (const double tol : {1e-3, 1e-9}) {
    const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({-1.2, 1.0})).with_objective(rosenbrock).with_tolerance(tol).solve();
    out.writeln("tolerance", tol, ": evaluations =", r.diagnostics.evaluations, ", minimum =", r.minimum);
  }
}
```

Output

```text
tolerance 0.001000000000 : evaluations = 143 , minimum = 0.000000100244
tolerance 0.000000001000 : evaluations = 231 , minimum = 0.000000000000
```

### Example 2

A population method, on the Rosenbrock function in a box. The generation count rises as the tolerance tightens.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(12));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  for (const double tol : {1e-2, 1e-6, 1e-10}) {
    const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({-1.2, 1.0})).with_objective(rosenbrock)
                                 .with_bounds(pluss::series::make({-3.0, -3.0}), pluss::series::make({3.0, 3.0}))
                                 .with_solver(dfo::DfoSolverKind::DifferentialEvolution).with_tolerance(tol).with_check_every(5).solve();
    out.writeln("tolerance", tol, ": generations =", r.diagnostics.iterations, ", minimum =", r.minimum);
  }
}
```

Output

```text
tolerance 0.010000000000 : generations = 85 , minimum = 0.000001224706
tolerance 0.000001000000 : generations = 100 , minimum = 0.000000006231
tolerance 0.000000000100 : generations = 105 , minimum = 0.000000006231
```

### Example 3

A mesh method.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(12));
  const auto f = [](const Series &x) { return (x - 3.0).square().sum(); };   // minimum 0 at (3, 3)

  for (const double tol : {1e-2, 1e-9}) {
    const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f).with_solver(dfo::DfoSolverKind::OrthoMads).with_tolerance(tol).solve();
    out.writeln("tolerance", tol, ": evaluations =", r.diagnostics.evaluations, ", minimum =", r.minimum);
  }
}
```

Output

```text
tolerance 0.010000000000 : evaluations = 327 , minimum = 0.000000000000
tolerance 0.000000001000 : evaluations = 402 , minimum = 0.000000000000
```

## See Also

- [`DfoBuilder`](../DfoBuilder.md): the class

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="with_solver.html">
    <span class="label">Previous</span>
    <span class="title">with_solver</span>
  </a>
</div>
<!-- apiref:nav:end -->
