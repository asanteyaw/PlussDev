<!-- generated from sine_transform.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::fourier::sine_transform`

The Fourier sine transform over the half line `[0, ∞)`.

## Signature

```cpp
operators::Operator operators::fourier::sine_transform(std::string_view variable, std::string_view frequency, operators::fourier::Convention convention = operators::fourier::Convention::Angular);
```

## What It Does

`sine_transform("x", "w")` takes `f(x)` for `x ≥ 0` and returns `S(w) = ∫₀^∞ f(x) sin(wx) dx`. It is the transform appropriate to a function extended *oddly* to the whole line: for an odd `f`, the full Fourier transform is `−2i` times the sine transform. It is the natural transform for half-line problems with a zero-value (Dirichlet) boundary condition.

Conventions and evaluation follow [`cosine_transform`](cosine_transform.md) and [`transform`](transform.md).

## Parameters

`std::string_view variable`
: The variable to integrate out, over `[0, ∞)`.

`std::string_view frequency`
: The name of the new frequency variable.

`operators::fourier::Convention convention`
: The convention; default `Angular`.

## Returns

An `Operator` whose application returns the transform as a `Function` of the frequency.

## Errors

- `the integration and frequency variables must be distinct, non-empty names`.
- An unknown variable raises `unknown variable '<name>'`.

## Practical Examples

### Example 1

`exp(−x)` has sine transform `w/(1 + w²)`. At `w = 2` that is `0.4`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace fourier = pluss::operators::fourier;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const math::Function f = math::function({"x"}, [](math::Function x) { return (-x).exp(); });
  const math::Function S = operators::fourier::sine_transform("x", "w")(f);
  out.writeln("S(2) =", S(2.0).item<double>(), "  2/(1 + 4) =", 2.0 / 5.0);
  out.writeln("S at w = 0, 1, 2:", S({pluss::make_series({0.0, 1.0, 2.0})}).items<double>());
}
```

Output

```text
S(2) = 0.40000   2/(1 + 4) = 0.40000
S at w = 0, 1, 2: [0.00000, 0.50000, 0.40000]
```

### Example 2

The unitary convention carries `√(2/π)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace fourier = pluss::operators::fourier;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const double pi = std::acos(-1.0);

  const math::Function f = math::function({"x"}, [](math::Function x) { return (-x).exp(); });
  const double value = operators::fourier::sine_transform("x", "w", fourier::Convention::Unitary)(f)(2.0).item<double>();
  out.writeln("Unitary S(2) =", value, "  sqrt(2/pi) * 2/5 =", std::sqrt(2.0 / pi) * 0.4);
}
```

Output

```text
Unitary S(2) = 0.31915   sqrt(2/pi) * 2/5 = 0.31915
```

### Example 3

For an odd function the full transform is `−2i` times the sine transform. Take `x exp(−x²)`: `F(0.7) = −i (√π w/2) e^{−w²/4}`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace fourier = pluss::operators::fourier;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const double pi = std::acos(-1.0);

  const math::Function g = math::function({"x"}, [](math::Function x) { return x * (-(x * x)).exp(); });
  const double w = 0.7;
  const double sine = operators::fourier::sine_transform("x", "w")(g)(w).item<double>();
  const std::complex<double> full = operators::fourier::transform("x", "w")(g)(w).item<std::complex<double>>();
  out.writeln("-2i S(0.7)  = 0 +", -2.0 * sine, "i");
  out.writeln("F(0.7)      =", full.real(), "+", full.imag(), "i   (-(sqrt(pi) w / 2) e^(-w^2/4) =", -std::sqrt(pi) * w / 2.0 * std::exp(-w * w / 4.0), ")");
}
```

Output

```text
-2i S(0.7)  = 0 + -0.54884 i
F(0.7)      = 0.00000 + -0.54884 i   (-(sqrt(pi) w / 2) e^(-w^2/4) = -0.54884 )
```

## See Also

- [`cosine_transform`](cosine_transform.md): the even counterpart
- [`transform`](transform.md): the full-line transform

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sine_series.html">
    <span class="label">Previous</span>
    <span class="title">sine_series</span>
  </a>
  <a class="next-link" href="transform.html">
    <span class="label">Next</span>
    <span class="title">transform</span>
  </a>
</div>
<!-- apiref:nav:end -->
