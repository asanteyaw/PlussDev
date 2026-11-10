<!-- generated from with_solver.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5dd4bbc0b823 -->

# `DfoBuilder::with_solver`

[Back to `dfo::DfoBuilder`](../DfoBuilder.md)

Chooses the method.

## Signature

```cpp
dfo::DfoBuilder dfo::DfoBuilder::with_solver(dfo::DfoSolverKind solver) const;
```

## What It Does

Sets `DfoOptions::solver`. The default `Auto` chooses from the problem (see [`DfoSolverKind`](../DfoSolverKind.md)); any other value forces that method.

## Parameters

`dfo::DfoSolverKind solver`
: The method, or `Auto`.

## Returns

A new builder with the change; the original is untouched.

## Practical Examples

### Example 1

Forcing Nelder-Mead.

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

  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f).with_solver(dfo::DfoSolverKind::NelderMead).solve();
  out.writeln(dfo::to_string(r.selected_solver), "|", r.selection_reason);
}
```

Output

```text
NelderMead | explicit solver override
```

### Example 2

Comparing methods on one problem.

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

  const dfo::DfoBuilder base = dfo::DFO::build(pluss::series::make({-1.2, 1.0})).with_objective(rosenbrock).with_max_evaluations(3000);
  for (const dfo::DfoSolverKind kind : {dfo::DfoSolverKind::NelderMead, dfo::DfoSolverKind::PatternSearch, dfo::DfoSolverKind::OrthoMads}) {
    const dfo::DfoResult r = base.with_solver(kind).solve();
    out.writeln(dfo::to_string(kind), ": minimum =", r.minimum, ", evaluations =", r.diagnostics.evaluations);
  }
}
```

Output

```text
NelderMead : minimum = 0.00000000 , evaluations = 159
PatternSearch : minimum = 0.00005153 , evaluations = 663
OrthoMads : minimum = 0.00086324 , evaluations = 1596
```

### Example 3

Back to automatic. `Auto` restores the choice by problem shape.

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

  const dfo::DfoBuilder forced = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f).with_solver(dfo::DfoSolverKind::PatternSearch);
  out.writeln(dfo::to_string(forced.solve().selected_solver), "->", dfo::to_string(forced.with_solver(dfo::DfoSolverKind::Auto).solve().selected_solver));
}
```

Output

```text
PatternSearch -> NelderMead
```

## See Also

- [`DfoBuilder`](../DfoBuilder.md): the class

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="with_seed.html">
    <span class="label">Previous</span>
    <span class="title">with_seed</span>
  </a>
  <a class="next-link" href="with_tolerance.html">
    <span class="label">Next</span>
    <span class="title">with_tolerance</span>
  </a>
</div>
<!-- apiref:nav:end -->
