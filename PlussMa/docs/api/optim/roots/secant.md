<!-- generated from secant.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `roots::secant`

The secant method: Newton's idea without a derivative, using the line through the last two iterates.

## Signature

```cpp
roots::Result roots::secant(const roots::Fn &f, pluss::Series x0, pluss::Series x1, const roots::Options &options = {});
```

## What It Does

`secant` replaces the derivative in Newton's step by the slope of the line through the last two points, `x₂ = x₁ − f(x₁)(x₁ − x₀)/(f(x₁) − f(x₀))`. It needs two starting values per lane but only one new evaluation of `f` per iteration, and converges with order about 1.6: faster than bisection, slower than Newton, and with no derivative to supply or approximate. Like Newton it is not bracketed and can wander if started far from a root.

`x0` and `x1` are `Series` of the same shape, one pair per lane. Where two successive function values are equal the denominator is replaced by 1 to avoid dividing by zero. `bracketed` in the [`Result`](Result.md) is always true.

## Parameters

`const roots::Fn &f`
: The function whose root is wanted.

`pluss::Series x0`
: The first starting value of each lane.

`pluss::Series x1`
: The second starting value, the same shape as `x0`.

`const roots::Options &options`
: Iteration limit, tolerances and cadence.

## Returns

A `roots::Result`: `root` (the latest iterate), `residual`, `iterations`; `bracketed` is always true.

## Practical Examples

### Example 1

Two starting points per lane. For `x² − 2` the pairs `(1, 2)`, `(1, 3)`, `(0.5, 4)` all converge to `√2`; with `x_tolerance` set, the loop stops once every lane's last step is below it (here at the second check, after 16 iterations).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace roots = pluss::subsp::math::roots;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(14));
  const roots::Fn f = [](const Series &x) { return x * x - 2.0; };   // root at sqrt(2)

  roots::Options options;
  options.x_tolerance = 1e-12;
  const roots::Result result = roots::secant(f, pluss::make_series({1.0, 1.0, 0.5}), pluss::make_series({2.0, 3.0, 4.0}), options);

  out.writeln("root       =", result.root.items<double>());
  out.writeln("residual   =", result.residual.items<double>());
  out.writeln("iterations =", result.iterations);
}
```

Output

```text
root       = [1.41421356237309, 1.41421356237309, 1.41421356237309]
residual   = [-0.00000000000000, -0.00000000000000, -0.00000000000000]
iterations = 16
```

### Example 2

Order of convergence. The residual after 1, 2, 3, 4, 5, 6 iterations from the pair `(1, 2)` shrinks faster than linearly but not as fast as Newton's squaring.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(12));
  const pluss::subsp::math::roots::Fn f = [](const Series &x) { return x * x - 2.0; };
  for (const std::int64_t iterations : {1, 2, 3, 4, 5, 6}) {
    pluss::subsp::math::roots::Options options;
    options.max_iterations = iterations;
    const auto result = pluss::subsp::math::roots::secant(f, pluss::make_series(1.0), pluss::make_series(2.0), options);
    out.writeln("iterations =", iterations, ", |f(x)| =", std::abs(result.residual.item<double>()));
  }
}
```

Output

```text
iterations = 1 , |f(x)| = 0.222222222222
iterations = 2 , |f(x)| = 0.040000000000
iterations = 3 , |f(x)| = 0.001189767995
iterations = 4 , |f(x)| = 0.000006007287
iterations = 5 , |f(x)| = 0.000000000893
iterations = 6 , |f(x)| = 0.000000000000
```

### Example 3

A transcendental equation with no convenient derivative: `eˣ = 3x` has two real roots (about `0.619` and `1.512`). The two sweeps start on either side of the minimum of `eˣ − 3x` and find one root each in a single call.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace roots = pluss::subsp::math::roots;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const roots::Fn f = [](const Series &x) { return x.exp() - x * 3.0; };
  roots::Options options;
  options.x_tolerance = 1e-12;
  const roots::Result result = roots::secant(f, pluss::make_series({0.0, 2.0}), pluss::make_series({0.5, 2.5}), options);

  out.writeln("roots    =", result.root.items<double>());
  out.writeln("residual =", result.residual.items<double>());
}
```

Output

```text
roots    = [0.6190612867, 1.5121345517]
residual = [0.0000000000, 0.0000000000]
```

## See Also

- [`newton`](newton.md): uses a derivative
- [`illinois`](illinois.md): bracketed and superlinear
- [`Options`](Options.md): iteration limit and tolerances

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="newton.html">
    <span class="label">Previous</span>
    <span class="title">newton</span>
  </a>
</div>
<!-- apiref:nav:end -->
