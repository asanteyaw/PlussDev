<!-- generated from QuadraScalarOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::QuadraScalarOptions`

Node count and adaptivity of the scalar Simpson rule.

`nodes` is the number of subintervals (rounded up to even, minimum 2); the rule makes `nodes + 1` calls. `adaptive` is **off** by default, so the rule is applied once at a fixed resolution; with it on, the nodes double until the change is within `tolerance` or `max_refinements` doublings have been made.

## Fields

### `nodes`

`std::int64_t`, default `256`.

Subintervals of the first level (rounded up to even).

### `adaptive`

`bool`, default `false`.

Double the nodes until the change is within `tolerance`.

### `tolerance`

`double`, default `1e-10`.

Convergence threshold.

### `max_refinements`

`std::int64_t`, default `8`.

The most doublings.

## Guidance

Passed to `quadra_scalar`.

## Practical Examples

### Example 1

The node count. More nodes, more accuracy, for `∫₀¹ e^{−x²} dx = 0.74682413`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  for (const std::int64_t nodes : {4, 16, 64}) {
    integ::QuadraScalarOptions options;
    options.nodes = nodes;
    const integ::QuadraScalarResult r = integ::quadra_scalar([](double t) { return std::exp(-t * t); }, 0.0, 1.0, options);
    out.writeln("nodes", nodes, ": value =", r.value, ", evaluations =", r.evaluations);
  }
}
```

Output

```text
nodes 4 : value = 0.7468553798 , evaluations = 5
nodes 16 : value = 0.7468242574 , evaluations = 17
nodes 64 : value = 0.7468241333 , evaluations = 65
```

### Example 2

Adaptive off and on. Off, the result is whatever the fixed rule gives; on, it is refined to the tolerance.

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
  for (const bool adaptive : {false, true}) {
    integ::QuadraScalarOptions options;
    options.nodes = 8;
    options.adaptive = adaptive;
    const integ::QuadraScalarResult r = integ::quadra_scalar([](double t) { return std::exp(-t * t); }, 0.0, 1.0, options);
    out.writeln("adaptive", adaptive, ": value =", r.value, ", converged =", r.converged, ", evaluations =", r.evaluations);
  }
}
```

Output

```text
adaptive false : value = 0.746826120527 , converged = true , evaluations = 9
adaptive true : value = 0.746824132814 , converged = true , evaluations = 510
```

### Example 3

A refinement budget. With `max_refinements = 1` a hard integrand is not converged, and the result says so.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  integ::QuadraScalarOptions options;
  options.adaptive = true;
  options.nodes = 4;
  options.max_refinements = 1;
  options.tolerance = 1e-12;
  const integ::QuadraScalarResult r = integ::quadra_scalar([](double t) { return std::sqrt(t); }, 0.0, 1.0, options);
  out.writeln("value =", r.value, ", error estimate =", r.error_estimate, ", converged =", r.converged);
}
```

Output

```text
value = 0.66307928 , error estimate = 0.00655302 , converged = false
```

## See Also

- [`quadra_scalar`](quadra_scalar.md): uses it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="QuadraRule.html">
    <span class="label">Previous</span>
    <span class="title">QuadraRule</span>
  </a>
  <a class="next-link" href="QuadraScalarResult.html">
    <span class="label">Next</span>
    <span class="title">QuadraScalarResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
