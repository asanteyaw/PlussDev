<!-- generated from with_initial.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 76bc03aa8a6c -->

# `DfoBuilder::with_initial`

[Back to `dfo::DfoBuilder`](../DfoBuilder.md)

Sets the starting point.

## Signature

```cpp
dfo::DfoBuilder dfo::DfoBuilder::with_initial(const pluss::Series &initial) const;
```

## What It Does

The start fixes the number of variables and, for the evolutionary and swarm methods, the centre of the sampling region when there are no bounds (`start ± 0.5` unless `initial_radius` says otherwise). It must be a real floating-point `Series` with at least one element.

## Parameters

`const pluss::Series &initial`
: The starting point.

## Returns

A new builder with the change; the original is untouched.

## Errors

- An undefined or empty start throws when the problem is solved.

## Practical Examples

### Example 1

Start from a chosen point.

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

  const dfo::DfoResult r = dfo::DFO::build().with_initial(pluss::series::make({10.0, -10.0})).with_objective(f).solve();
  out.writeln("minimizer =", r.minimizer.items<double>());
}
```

Output

```text
minimizer = [3.0000, 3.0000]
```

### Example 2

Different starts, one builder. The base holds the objective; each start is a branch.

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

  const dfo::DfoBuilder base = dfo::DFO::build().with_objective(f);
  for (const double s : {-5.0, 0.0, 8.0}) {
    const dfo::DfoResult r = base.with_initial(pluss::series::make({s, s})).solve();
    out.writeln("start", s, ": minimizer =", r.minimizer.items<double>(), ", evaluations =", r.diagnostics.evaluations);
  }
}
```

Output

```text
start -5.0000 : minimizer = [3.0000, 3.0000] , evaluations = 81
start 0.0000 : minimizer = [3.0000, 3.0000] , evaluations = 82
start 8.0000 : minimizer = [3.0000, 3.0000] , evaluations = 72
```

### Example 3

The start sets the dimension. A six-variable start is solved by the same objective; the simplex gets each coordinate to about two digits.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x - 1.0).square().sum(); };
  const dfo::DfoResult r = dfo::DFO::build().with_initial(pluss::series::make({0.0, 0.0, 0.0, 0.0, 0.0, 0.0})).with_objective(f).solve();
  out.writeln("dimension =", r.minimizer.numel(), ", minimizer =", r.minimizer.items<double>());
}
```

Output

```text
dimension = 6 , minimizer = [1.0110, 1.0103, 1.0077, 0.9918, 0.9617, 1.0154]
```

## See Also

- [`DfoBuilder`](../DfoBuilder.md): the class

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="with_display_iter.html">
    <span class="label">Previous</span>
    <span class="title">with_display_iter</span>
  </a>
  <a class="next-link" href="with_mads_config.html">
    <span class="label">Next</span>
    <span class="title">with_mads_config</span>
  </a>
</div>
<!-- apiref:nav:end -->
