<!-- generated from with_bounds.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a878bd0b77b3 -->

# `DfoBuilder::with_bounds`

[Back to `dfo::DfoBuilder`](../DfoBuilder.md)

Sets lower and upper bounds on every variable.

## Signature

```cpp
dfo::DfoBuilder dfo::DfoBuilder::with_bounds(const pluss::Series &lower, const pluss::Series &upper) const;
```

## What It Does

`lower` and `upper` each have one element per variable and are set together. Nelder-Mead enforces them by projection (the objective is never called outside the box), pattern search and the mesh methods keep their points inside, and the evolutionary and swarm methods sample inside the box. With bounds and no constraints, `Auto` chooses differential evolution.

## Parameters

`const pluss::Series &lower`
: Lower bound per variable.

`const pluss::Series &upper`
: Upper bound per variable.

## Returns

A new builder with the change; the original is untouched.

## Errors

- Bounds whose length differs from the start throw when the problem is solved.

## Practical Examples

### Example 1

A box that cuts off the optimum. The unconstrained minimum `(3, 3)` lies outside `[0, 2]²`; the answer is the corner.

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

  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f).with_bounds(pluss::series::make({0.0, 0.0}), pluss::series::make({2.0, 2.0})).solve();
  out.writeln("minimizer =", r.minimizer.items<double>(), ", solver =", dfo::to_string(r.selected_solver));
}
```

Output

```text
minimizer = [2.0000, 2.0000] , solver = DifferentialEvolution
```

### Example 2

Bounds with a forced method. Nelder-Mead projects every trial point into the box.

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

  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f).with_bounds(pluss::series::make({0.0, 0.0}), pluss::series::make({2.0, 2.0})).with_solver(dfo::DfoSolverKind::NelderMead).solve();
  out.writeln("minimizer =", r.minimizer.items<double>(), ", solver =", dfo::to_string(r.selected_solver));
}
```

Output

```text
minimizer = [2.0000, 2.0000] , solver = NelderMead
```

### Example 3

One variable bounded, the other nearly free.

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

  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f)
                               .with_bounds(pluss::series::make({-100.0, -100.0}), pluss::series::make({1.0, 100.0}))
                               .with_solver(dfo::DfoSolverKind::PatternSearch)
                               .solve();
  out.writeln("minimizer =", r.minimizer.items<double>());
}
```

Output

```text
minimizer = [1.0000, 2.9999]
```

## See Also

- [`DfoBuilder`](../DfoBuilder.md): the class

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="with_blackbox.html">
    <span class="label">Previous</span>
    <span class="title">with_blackbox</span>
  </a>
  <a class="next-link" href="with_check_every.html">
    <span class="label">Next</span>
    <span class="title">with_check_every</span>
  </a>
</div>
<!-- apiref:nav:end -->
