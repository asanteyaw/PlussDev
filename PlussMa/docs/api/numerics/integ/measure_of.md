<!-- generated from measure_of.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::measure_of`

The Lebesgue measure of the set where a function exceeds a level: the length of `{x : f(x) > level}`.

## Signature

```cpp
integ::MeasureResult integ::measure_of(const integ::IntegrandFn &fn, double a, double b, double level, const integ::LebesgueOptions &options = {});
```

## What It Does

Returns the total length of the part of `[a, b]` on which `fn(x) > level`. The boundary of the set is located **inside** a cell by linear interpolation between neighbouring cell midpoints, so the length is second-order accurate in the cell width. The cells refine, as in [`lebesgue`](lebesgue.md), until the measure stops changing. The measure of a super-level set is the building block of the layer-cake formula `∫ f = ∫₀^∞ μ{f > t} dt`.

## Parameters

`const integ::IntegrandFn &fn`
: `Series -> Series`.

`double a, double b`
: A finite interval.

`double level`
: The threshold.

`const integ::LebesgueOptions &options`
: Cells, refinements, tolerance.

## Returns

A [`MeasureResult`](MeasureResult.md).

## Errors

- A missing integrand or an invalid interval throws.

## Practical Examples

### Example 1

A sub-interval. `x² > 1/4` on `[0, 1]` holds for `x > 1/2`, so the measure is `1/2`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const integ::MeasureResult r = integ::measure_of([](const Series &t) { return t.square(); }, 0.0, 1.0, 0.25);
  out.writeln("measure =", r.value.item<double>(), ", converged =", r.converged);
}
```

Output

```text
measure = 0.50000000 , converged = true
```

### Example 2

Several pieces. `sin x > 1/2` on `[0, 2π]` holds on `(π/6, 5π/6)`, of length `2π/3 = 2.0944`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const integ::MeasureResult r = integ::measure_of([](const Series &t) { return t.sin(); }, 0.0, 2.0 * 3.14159265358979323846, 0.5);
  out.writeln("measure =", r.value.item<double>(), ", exact =", 2.0 * 3.14159265358979323846 / 3.0);
}
```

Output

```text
measure = 2.094395 , exact = 2.094395
```

### Example 3

The layer-cake check. Summing the measures of `{x² > t}` over levels `t` reproduces `∫₀¹ x² dx = 1/3`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  double integral = 0.0;
  const int levels = 200;
  for (int i = 0; i < levels; ++i) {
    const double t = (i + 0.5) / levels;
    integral += integ::measure_of([](const Series &s) { return s.square(); }, 0.0, 1.0, t).value.item<double>() / levels;
  }
  out.writeln("layer-cake integral =", integral, ", exact =", 1.0 / 3.0);
}
```

Output

```text
layer-cake integral = 0.333312 , exact = 0.333333
```

## See Also

- [`MeasureResult`](MeasureResult.md): what it returns
- [`lebesgue`](lebesgue.md): the integral

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lebesgue.html">
    <span class="label">Previous</span>
    <span class="title">lebesgue</span>
  </a>
  <a class="next-link" href="quadra.html">
    <span class="label">Next</span>
    <span class="title">quadra</span>
  </a>
</div>
<!-- apiref:nav:end -->
