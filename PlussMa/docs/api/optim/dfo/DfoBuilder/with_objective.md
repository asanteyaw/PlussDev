<!-- generated from with_objective.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: debe5d96020c -->

# `DfoBuilder::with_objective`

[Back to `dfo::DfoBuilder`](../DfoBuilder.md)

Sets the objective, a function of the point that returns a real scalar.

## Signature

```cpp
dfo::DfoBuilder dfo::DfoBuilder::with_objective(std::function<pluss::Series(const pluss::Series &)> objective) const;
template <class Fn> dfo::DfoBuilder dfo::DfoBuilder::with_objective(Fn &&objective) const;
```

## What It Does

Accepts a `std::function` or any callable that returns a `Series` or an arithmetic type. A number is wrapped as a scalar `Series`. The objective may be non-smooth or discontinuous, and may use any computation, since nothing is differentiated. Give an objective **or** a black box, not both.

## Parameters

`std::function<pluss::Series(const pluss::Series &)> objective`
: The objective, or any callable returning a `Series` or a number.

## Returns

A new builder with the change; the original is untouched.

## Errors

- A result that is not a real scalar throws on the first evaluation.

## Practical Examples

### Example 1

A Series-returning lambda.

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

  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f).solve();
  out.writeln("minimum =", r.minimum);
}
```

Output

```text
minimum = 0.0000
```

### Example 2

A number-returning lambda. The element values are read out and combined with ordinary arithmetic.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) {
    const double a = x.select(0, 0).item<double>(), b = x.select(0, 1).item<double>();
    return std::abs(a - 2.0) + std::abs(b + 1.0);
  };
  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f).solve();
  out.writeln("minimizer =", r.minimizer.items<double>(), ", minimum =", r.minimum);
}
```

Output

```text
minimizer = [2.0000, -1.0000] , minimum = 0.0000
```

### Example 3

A discontinuous objective. A step function has no gradient; the search still descends it to the plateau of lowest value.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x * 4.0).floor().abs().sum(); };
  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f).solve();
  out.writeln("minimum =", r.minimum, ", minimizer in the flat region:", r.minimizer.items<double>());
}
```

Output

```text
minimum = 0.0000 , minimizer in the flat region: [0.0000, 0.0000]
```

## See Also

- [`DfoBuilder`](../DfoBuilder.md): the class

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="with_nelder_mead_config.html">
    <span class="label">Previous</span>
    <span class="title">with_nelder_mead_config</span>
  </a>
  <a class="next-link" href="with_options.html">
    <span class="label">Next</span>
    <span class="title">with_options</span>
  </a>
</div>
<!-- apiref:nav:end -->
