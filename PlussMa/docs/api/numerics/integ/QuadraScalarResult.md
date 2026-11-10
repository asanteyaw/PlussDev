<!-- generated from QuadraScalarResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::QuadraScalarResult`

The value of a scalar integral with its error estimate.

`value` is a plain `double`; `error_estimate` the change between the last two levels (zero when `adaptive` is off); `evaluations` the number of calls to the function; `converged` whether the change fell within the tolerance (always true for an interval of zero length).

## Fields

### `value`

`double`, default `0`.

The integral.

### `error_estimate`

`double`, default `0`.

Change between the last two levels.

### `evaluations`

`std::int64_t`, default `0`.

Calls of the function.

### `converged`

`bool`, default `false`.

Whether the change fell within the tolerance.

## Guidance

Returned by `quadra_scalar`.

## Practical Examples

### Example 1

Reading a result.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(12));
  const integ::QuadraScalarResult r = integ::quadra_scalar([](double t) { return std::cos(t); }, 0.0, 1.0);
  out.writeln("value =", r.value, ", exact =", std::sin(1.0), ", evaluations =", r.evaluations);
}
```

Output

```text
value = 0.841470984809 , exact = 0.841470984808 , evaluations = 257
```

### Example 2

An empty interval. `b ≤ a` returns zero and counts as converged.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const integ::QuadraScalarResult r = integ::quadra_scalar([](double t) { return t; }, 1.0, 1.0);
  out.writeln("value =", r.value, ", converged =", r.converged, ", evaluations =", r.evaluations);
}
```

Output

```text
value = 0.0000 , converged = true , evaluations = 0
```

### Example 3

The estimate shrinks with refinement.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(14));
  integ::QuadraScalarOptions options;
  options.adaptive = true;
  options.nodes = 8;
  for (const double tol : {1e-4, 1e-10}) {
    options.tolerance = tol;
    const integ::QuadraScalarResult r = integ::quadra_scalar([](double t) { return std::exp(t); }, 0.0, 1.0, options);
    out.writeln("tolerance", tol, ": error estimate =", r.error_estimate, ", evaluations =", r.evaluations);
  }
}
```

Output

```text
tolerance 0.00010000000000 : error estimate = 0.00000218064801 , evaluations = 26
tolerance 0.00000000010000 : error estimate = 0.00000000003334 , evaluations = 510
```

## See Also

- [`quadra_scalar`](quadra_scalar.md): produces it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="QuadraScalarOptions.html">
    <span class="label">Previous</span>
    <span class="title">QuadraScalarOptions</span>
  </a>
  <a class="next-link" href="aleatoria.html">
    <span class="label">Next</span>
    <span class="title">aleatoria</span>
  </a>
</div>
<!-- apiref:nav:end -->
