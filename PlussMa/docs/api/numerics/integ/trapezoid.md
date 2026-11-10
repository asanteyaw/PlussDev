<!-- generated from trapezoid.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::trapezoid`

Integrates sampled values by the trapezoidal rule, along one axis.

## Signature

```cpp
pluss::Series integ::trapezoid(const pluss::Series &y, const pluss::Series &x, std::int64_t dim = -1);
pluss::Series integ::trapezoid(const pluss::Series &y, double dx, std::int64_t dim = -1);
pluss::Series integ::trapezoid(const pluss::Series &y, std::int64_t dim = -1);
```

## What It Does

For values `y` measured at points `x`, the trapezoidal rule adds up the areas of the trapezoids between neighbouring points, `Σ (xᵢ₊₁ − xᵢ)(yᵢ + yᵢ₊₁)/2`. The three overloads differ in how the spacing is given: an explicit `Series` of points (uneven spacing is fine), one constant spacing `dx`, or nothing, which means unit spacing. `dim` is the axis the integral runs along; every other axis is a batch, so one call integrates many rows at once.

The rule is exact for functions that are linear between samples and has an error of order `dx²` for smooth ones. It works on any real or complex `Series`, and it is built from ordinary differentiable operations, so a gradient flows through it. For a function you can evaluate wherever you like, [`quadra`](quadra.md) is far more accurate for the same number of evaluations.

## Parameters

`const pluss::Series &y`
: The sampled values, any real or complex shape.

`const pluss::Series &x`
: The sample points along `dim`, same length as `y` there.

`double dx`
: A constant spacing.

`std::int64_t dim`
: The axis to integrate along; negative counts from the end. Default last.

## Returns

A `Series` with `dim` removed: one integral per remaining index.

## Errors

- Mismatched lengths of `x` and `y` along `dim`, or fewer than two samples, throw.

## Practical Examples

### Example 1

Uneven points. The integral of `x²` over `[0, 1]` is `1/3`; ten trapezoids give `0.335`, the error of order `dx²`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const Series x = pluss::series::linspace(0.0, 1.0, 11, pluss::ScalarType::Float64);
  out.writeln("integral =", integ::trapezoid(x.square(), x).item<double>(), ", exact =", 1.0 / 3.0);
}
```

Output

```text
integral = 0.335000 , exact = 0.333333
```

### Example 2

Constant spacing, and the default. With a spacing of `0.1` the result is the same; with no spacing at all the points are one apart, so the integral is ten times larger.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const Series x = pluss::series::linspace(0.0, 1.0, 11, pluss::ScalarType::Float64);   // 0, 0.1, ..., 1

  const Series y = x.square();
  out.writeln("dx = 0.1 :", integ::trapezoid(y, 0.1).item<double>());
  out.writeln("default  :", integ::trapezoid(y).item<double>());
}
```

Output

```text
dx = 0.1 : 0.335000
default  : 3.350000
```

### Example 3

Many rows at once. Each row of a matrix is its own signal; `dim` picks the axis to integrate. The rows `sin(kx)` over `[0, π]` integrate to `2/k` for odd `k`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const Series x = pluss::series::linspace(0.0, 3.14159265358979323846, 2001, pluss::ScalarType::Float64);
  const Series k = pluss::series::make({1.0, 3.0, 5.0}).reshape({3, 1});
  const Series y = (k * x.reshape({1, 2001})).sin();
  out.writeln("integrals =", integ::trapezoid(y, x, 1).items<double>(), ", exact =", 2.0, 2.0 / 3.0, 0.4);
}
```

Output

```text
integrals = [2.00000, 0.66667, 0.40000] , exact = 2.00000 0.66667 0.40000
```

### Example 4

A gradient through the integral. With `y = a x²` the integral is `a/3`, so its derivative with respect to `a` is `1/3`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const Series x = pluss::series::linspace(0.0, 1.0, 1001, pluss::ScalarType::Float64);
  Series a = pluss::make_series(2.0);
  a.tracks_slope_(true);
  const Series integral = integ::trapezoid(a * x.square(), x);
  integral.sum().reverse();
  out.writeln("integral =", integral.item<double>(), ", d/da =", a.sensitivity().item<double>());
}
```

Output

```text
integral = 0.666667 , d/da = 0.333334
```

### Example 5

Complex values. `∫₀^π e^{ix} dx = (e^{iπ} − 1)/i = 2i`; the rule is a linear combination, so it works on complex data unchanged.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const Series x = pluss::series::linspace(0.0, 3.14159265358979323846, 4001, pluss::ScalarType::Float64);
  const Series i = pluss::make_series(std::vector<std::complex<double>>{{0.0, 1.0}}, {1}).to(pluss::ScalarType::Complex128);   // the imaginary unit
  const Series y = (x.to(pluss::ScalarType::Complex128) * i).exp();
  const std::complex<double> v = integ::trapezoid(y, x.to(pluss::ScalarType::Complex128)).item<std::complex<double>>();
  out.writeln("integral =", v.real(), v.imag(), ", exact = 0, 2");
}
```

Output

```text
integral = -0.000000 2.000000 , exact = 0, 2
```

### Example 6

An axis other than the last. The columns of a matrix are the signals, so `dim = 0` integrates down each column: `x` and `x²` over `[0, 1]` give `1/2` and `1/3`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const Series x = pluss::series::linspace(0.0, 1.0, 1001, pluss::ScalarType::Float64);
  const Series y = x.reshape({1001, 1}).unite({x.square().reshape({1001, 1})}, 1);   // two columns
  out.writeln("integrals =", integ::trapezoid(y, x, 0).items<double>(), ", exact = 0.5, 0.33333333");
}
```

Output

```text
integrals = [0.500000, 0.333333] , exact = 0.5, 0.33333333
```

## See Also

- [`cumulative_trapezoid`](cumulative_trapezoid.md): the running integral
- [`quadra`](quadra.md): integrating a function

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="quadra_scalar.html">
    <span class="label">Previous</span>
    <span class="title">quadra_scalar</span>
  </a>
</div>
<!-- apiref:nav:end -->
