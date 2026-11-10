<!-- generated from IntegraResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::IntegraResult`

The value `integra` found, its error estimate and which engine produced it.

`value` is the integral; `error_estimate` is the quadrature change or the Monte Carlo standard error, whichever engine ran; `evaluations` counts every function evaluation spent, including a failed quadrature attempt before a Monte Carlo fallback; `strategy_used` is the engine; `converged` is true when quadrature converged or Monte Carlo ran, and false when quadrature missed its tolerance but was still more accurate than Monte Carlo (the quadrature value is then returned).

## Fields

### `value`

`pluss::Series`, default `undefined`.

The integral.

### `error_estimate`

`double`, default `0`.

Quadrature change or Monte Carlo standard error.

### `evaluations`

`std::int64_t`, default `0`.

Function evaluations spent.

### `strategy_used`

`integ::IntegraStrategy`, default `Quadrature`.

The engine that produced `value`.

### `converged`

`bool`, default `false`.

Whether the estimate met its engine's convergence test.

## Guidance

Returned by `integra`.

## Practical Examples

### Example 1

Reading a result.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const integ::IntegraResult r = integ::integra([](const Series &t) { return t.cos(); }, 0.0, 1.0);
  out.writeln("value =", r.value.item<double>(), ", error estimate =", r.error_estimate, ", evaluations =", r.evaluations, ", converged =", r.converged);
}
```

Output

```text
value = 0.8414709848 , error estimate = 0.0000000000 , evaluations = 774 , converged = true
```

### Example 2

The engine that ran.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const integ::IntegraResult r = integ::integra([](const Series &t) { return t.exp(); }, 0.0, 1.0);
  out.writeln(r.strategy_used == integ::IntegraStrategy::Quadrature ? "quadrature" : "monte carlo");
}
```

Output

```text
quadrature
```

### Example 3

A result that did not converge is still usable. The estimate says how accurate it is.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(12));
  integ::IntegraOptions options;
  options.quadrature.tolerance = 1e-16;
  options.quadrature.max_refinements = 2;
  options.monte_carlo.samples = 1000;
  const integ::IntegraResult r = integ::integra([](const Series &t) { return t.exp(); }, 0.0, 1.0, options);
  out.writeln("converged =", r.converged, ", strategy =", static_cast<int>(r.strategy_used), ", error estimate =", r.error_estimate, ", value =", r.value.item<double>());
}
```

Output

```text
converged = false , strategy = 0 , error estimate = 0.000000000000 , value = 1.718281828459
```

## See Also

- [`integra`](integra.md): produces it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="IntegraOptions.html">
    <span class="label">Previous</span>
    <span class="title">IntegraOptions</span>
  </a>
  <a class="next-link" href="IntegraStrategy.html">
    <span class="label">Next</span>
    <span class="title">IntegraStrategy</span>
  </a>
</div>
<!-- apiref:nav:end -->
