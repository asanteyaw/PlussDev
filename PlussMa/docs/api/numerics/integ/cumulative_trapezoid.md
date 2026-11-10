<!-- generated from cumulative_trapezoid.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::cumulative_trapezoid`

The running trapezoidal integral of sampled values: the area up to each point.

## Signature

```cpp
pluss::Series integ::cumulative_trapezoid(const pluss::Series &y, const pluss::Series &x, std::int64_t dim = -1);
pluss::Series integ::cumulative_trapezoid(const pluss::Series &y, double dx, std::int64_t dim = -1);
pluss::Series integ::cumulative_trapezoid(const pluss::Series &y, std::int64_t dim = -1);
```

## What It Does

Where [`trapezoid`](trapezoid.md) returns one number, `cumulative_trapezoid` returns the integral from the first sample up to each later one, so the result has **one fewer element** than `y` along `dim` (there is no area up to the first point) and its last element equals the full integral. The three overloads take the spacing exactly as `trapezoid` does. It is the discrete antiderivative: applied to `sin` it gives `1 − cos`, to an acceleration it gives a velocity, to a density it gives a distribution function.

## Parameters

`const pluss::Series &y`
: The sampled values.

`const pluss::Series &x`
: The sample points along `dim`.

`double dx`
: A constant spacing.

`std::int64_t dim`
: The axis; default last.

## Returns

A `Series` shorter by one along `dim`.

## Errors

- Mismatched lengths or fewer than two samples throw.

## Practical Examples

### Example 1

The antiderivative of `2x`. The running integral is `x²` at every point after the first.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const Series x = pluss::series::linspace(0.0, 1.0, 11, pluss::ScalarType::Float64);   // 0, 0.1, ..., 1

  const Series running = integ::cumulative_trapezoid(x * 2.0, x);
  out.writeln("running =", running.items<double>());
  out.writeln("x²      =", x.slice(0, 1, 11, 1).square().items<double>());
}
```

Output

```text
running = [0.010, 0.040, 0.090, 0.160, 0.250, 0.360, 0.490, 0.640, 0.810, 1.000]
x²      = [0.010, 0.040, 0.090, 0.160, 0.250, 0.360, 0.490, 0.640, 0.810, 1.000]
```

### Example 2

A distribution function from a density. The standard normal density, integrated along a fine grid, reaches `Φ(2) = 0.97725` at 2.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const Series x = pluss::series::linspace(-8.0, 8.0, 3201, pluss::ScalarType::Float64);
  const Series density = (x.square() * -0.5).exp() / std::sqrt(2.0 * 3.14159265358979323846);
  const Series cdf = integ::cumulative_trapezoid(density, x);
  // cdf has 3200 elements; element i is the area up to x[i + 1]. x = 2 is point 2000, so element 1999.
  out.writeln("Phi(2) =", cdf.select(0, 1999).item<double>(), ", total =", cdf.select(0, 3199).item<double>());
}
```

Output

```text
Phi(2) = 0.977250 , total = 1.000000
```

### Example 3

Constant spacing and unit spacing. The running sum of `1, 2, 3, 4` with unit spacing is `1.5, 4, 7.5`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const Series y = pluss::series::make({1.0, 2.0, 3.0, 4.0});
  out.writeln("unit spacing :", integ::cumulative_trapezoid(y).items<double>());
  out.writeln("dx = 0.5     :", integ::cumulative_trapezoid(y, 0.5).items<double>());
}
```

Output

```text
unit spacing : [1.500, 4.000, 7.500]
dx = 0.5     : [0.750, 2.000, 3.750]
```

### Example 4

Uneven points. The points `0, 1, 3, 6` with a constant integrand 1 accumulate the distances: the area up to each later point is `1, 3, 6`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const Series x = pluss::series::make({0.0, 1.0, 3.0, 6.0});
  const Series y = pluss::series::make({1.0, 1.0, 1.0, 1.0});
  out.writeln("running =", integ::cumulative_trapezoid(y, x).items<double>());
}
```

Output

```text
running = [1.000, 3.000, 6.000]
```

### Example 5

An axis other than the last. A matrix with one signal per row, integrated along the rows (`dim = 1`): each row `[1, 2, 3]` and `[0, 0, 4]` accumulates independently.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const Series y = pluss::series::make({1.0, 2.0, 3.0, 0.0, 0.0, 4.0}).reshape({2, 3});
  const Series running = integ::cumulative_trapezoid(y, 1.0, 1);
  out.writeln("shape =", running.sizes(), ", values =", running.reshape({running.numel()}).items<double>());
}
```

Output

```text
shape = [2, 2] , values = [1.500, 4.000, 0.000, 2.000]
```

### Example 6

A gradient through the running integral. For `y = a·[1, 2, 3]` the last running value is `a (1.5 + 2.5) = 4a`, so its derivative with respect to `a` is 4.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  Series a = pluss::make_series(1.0);
  a.tracks_slope_(true);
  const Series y = a * pluss::series::make({1.0, 2.0, 3.0});
  const Series last = integ::cumulative_trapezoid(y).select(0, 1);
  last.reverse();
  out.writeln("last =", last.item<double>(), ", d/da =", a.sensitivity().item<double>());
}
```

Output

```text
last = 4.000 , d/da = 4.000
```

## See Also

- [`trapezoid`](trapezoid.md): the total

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="analytica_sine.html">
    <span class="label">Previous</span>
    <span class="title">sine</span>
  </a>
  <a class="next-link" href="integra.html">
    <span class="label">Next</span>
    <span class="title">integra</span>
  </a>
</div>
<!-- apiref:nav:end -->
