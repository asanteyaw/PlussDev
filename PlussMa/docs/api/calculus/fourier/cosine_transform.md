<!-- generated from cosine_transform.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::fourier::cosine_transform`

The Fourier cosine transform over the half line `[0, ∞)`.

## Signature

```cpp
operators::Operator operators::fourier::cosine_transform(std::string_view variable, std::string_view frequency, operators::fourier::Convention convention = operators::fourier::Convention::Angular);
```

## What It Does

`cosine_transform("x", "w")` takes `f(x)` for `x ≥ 0` and returns `C(w) = ∫₀^∞ f(x) cos(wx) dx`, a function of the new variable `w`. It is the transform appropriate to a function extended evenly to the whole line: for an even `f`, the full Fourier transform is twice the cosine transform. It is also the natural transform for problems on a half line with a zero-derivative boundary condition.

The conventions follow [`transform`](transform.md): `Angular` has no prefactor, `Unitary` carries `√(2/π)`, and `Ordinary` uses `2πξ` in the phase and a factor 2. The same rules apply for closed forms (exponential polynomials give exact results) and for numerical evaluation otherwise. Names must be distinct.

## Parameters

`std::string_view variable`
: The variable to integrate out, over `[0, ∞)`.

`std::string_view frequency`
: The name of the new frequency variable; must differ from `variable` and every other variable.

`operators::fourier::Convention convention`
: The convention; default `Angular`.

## Returns

An `Operator` whose application returns the transform as a `Function` of the frequency.

## Errors

- `the integration and frequency variables must be distinct, non-empty names`.
- An unknown variable raises `unknown variable '<name>'`.

## Practical Examples

### Example 1

`exp(−x)` has cosine transform `1/(1 + w²)`. At `w = 2` that is `0.2`.

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
  const math::Function C = operators::fourier::cosine_transform("x", "w")(f);
  out.writeln("C(2) =", C(2.0).item<double>(), "  1/(1 + 4) =", 1.0 / 5.0);
  out.writeln("C at w = 0, 1, 2:", C({pluss::make_series({0.0, 1.0, 2.0})}).items<double>());
}
```

Output

```text
C(2) = 0.20000   1/(1 + 4) = 0.20000
C at w = 0, 1, 2: [1.00000, 0.50000, 0.20000]
```

### Example 2

The other conventions. Unitary carries `√(2/π)`: `√(2/π)/5` at `w = 2`. Ordinary uses `2πξ` and a factor 2: `2/(1 + (2πξ)²)`.

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
  out.writeln("Unitary at 2:     ", operators::fourier::cosine_transform("x", "w", fourier::Convention::Unitary)(f)(2.0).item<double>(), "  sqrt(2/pi)/5 =", std::sqrt(2.0 / pi) / 5.0);
  out.writeln("Ordinary at 0.3:  ", operators::fourier::cosine_transform("x", "w", fourier::Convention::Ordinary)(f)(0.3).item<double>(), "  2/(1 + (2 pi 0.3)^2) =", 2.0 / (1.0 + std::pow(2.0 * pi * 0.3, 2)));
}
```

Output

```text
Unitary at 2:      0.15958   sqrt(2/pi)/5 = 0.15958
Ordinary at 0.3:   0.43927   2/(1 + (2 pi 0.3)^2) = 0.43927
```

### Example 3

For an even function the full transform is twice the cosine transform. The Gaussian `exp(−x²)`: `2·C(0.7)` equals `F(0.7)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace fourier = pluss::operators::fourier;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const math::Function g = math::function({"x"}, [](math::Function x) { return (-(x * x)).exp(); });
  const double cosine = operators::fourier::cosine_transform("x", "w")(g)(0.7).item<double>();
  const double full = operators::fourier::transform("x", "w")(g)(0.7).item<std::complex<double>>().real();
  out.writeln("2 * C(0.7) =", 2.0 * cosine);
  out.writeln("F(0.7)     =", full);
}
```

Output

```text
2 * C(0.7) = 1.56810
F(0.7)     = 1.56810
```

## See Also

- [`sine_transform`](sine_transform.md): the odd counterpart
- [`transform`](transform.md): the full-line transform

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cosine_series.html">
    <span class="label">Previous</span>
    <span class="title">cosine_series</span>
  </a>
  <a class="next-link" href="inverse_transform.html">
    <span class="label">Next</span>
    <span class="title">inverse_transform</span>
  </a>
</div>
<!-- apiref:nav:end -->
