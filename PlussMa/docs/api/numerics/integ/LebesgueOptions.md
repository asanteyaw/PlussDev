<!-- generated from LebesgueOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::LebesgueOptions`

Cell count, refinement budget, grading and tolerance of Lebesgue integration.

`cells` is the starting number of cells and doubles at each refinement, up to `max_refinements` times, until the value stops changing. Cell boundaries are `x = a + (b − a)·uᵍ` on a uniform `u`; `grading` `g > 1` concentrates cells at `a`, which resolves an endpoint singularity such as `1/√x` on `(0, 1]`. `tolerance` bounds the width of the bracket.

## Fields

### `cells`

`std::int64_t`, default `1024`.

Starting cell count, doubled at each refinement.

### `max_refinements`

`std::int64_t`, default `8`.

The most doublings.

### `grading`

`double`, default `1`.

Cell concentration at `a`; above 1 grades toward `a`.

### `tolerance`

`double`, default `1e-8`.

Bound on the bracket width.

### `dtype`

`pluss::ScalarType`, default `Float64`.

Precision.

### `platform`

`pluss::Platform`, default `CPU`.

Where the cells live.

## Guidance

Passed to `lebesgue`.

## Practical Examples

### Example 1

Grading toward a singular end. Without grading the integral of `x^(−1/2)` on `(0, 1]` converges slowly; with grading it converges within the same budget.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  for (const double grading : {1.0, 3.0}) {
    integ::LebesgueOptions options;
    options.grading = grading;
    options.tolerance = 1e-5;
    options.max_refinements = 8;
    const integ::LebesgueResult r = integ::lebesgue([](const Series &t) { return t.pow(-0.5); }, 0.0, 1.0, options);
    out.writeln("grading", grading, ": value =", r.value.item<double>(), ", converged =", r.converged);
  }
}
```

Output

```text
grading 1.000000 : value = 1.998819 , converged = false
grading 3.000000 : value = 2.000005 , converged = true
```

### Example 2

The tolerance. The first two tolerances are met by the same refinement level, so they cost the same; the tightest needs more levels.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  for (const double tol : {1e-3, 1e-6, 1e-9}) {
    integ::LebesgueOptions options;
    options.tolerance = tol;
    const integ::LebesgueResult r = integ::lebesgue([](const Series &t) { return t.sin().abs(); }, 0.0, 3.14159265358979323846, options);
    out.writeln("tolerance", tol, ": bracket width =", r.upper.item<double>() - r.lower.item<double>(), ", evaluations =", r.evaluations);
  }
}
```

Output

```text
tolerance 0.0010000000 : bracket width = 0.0002499843 , evaluations = 3072
tolerance 0.0000010000 : bracket width = 0.0000002500 , evaluations = 3072
tolerance 0.0000000010 : bracket width = 0.0000000002 , evaluations = 130048
```

### Example 3

The starting cells and the refinement budget.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(9));
  integ::LebesgueOptions options;
  options.cells = 64;
  options.max_refinements = 3;
  const integ::LebesgueResult r = integ::lebesgue([](const Series &t) { return t.exp(); }, 0.0, 1.0, options);
  out.writeln("value =", r.value.item<double>(), ", refinements =", r.refinements, ", history size =", r.history.size());
}
```

Output

```text
value = 1.718281555 , refinements = 3 , history size = 4
```

## See Also

- [`lebesgue`](lebesgue.md): uses it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="IntegraStrategy.html">
    <span class="label">Previous</span>
    <span class="title">IntegraStrategy</span>
  </a>
  <a class="next-link" href="LebesgueResult.html">
    <span class="label">Next</span>
    <span class="title">LebesgueResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
