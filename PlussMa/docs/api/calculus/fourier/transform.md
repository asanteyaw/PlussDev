<!-- generated from transform.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::fourier::transform`

The Fourier transform of a function over the real line, as a function of a new frequency variable.

## Signature

```cpp
operators::Operator operators::fourier::transform(std::string_view variable, std::string_view frequency, operators::fourier::Convention convention = operators::fourier::Convention::Angular);
```

## What It Does

`transform("x", "w")` takes a function of `x` and returns its Fourier transform: a function of a *new* variable `w`, in which `x` has been integrated out. The transform decomposes a function into frequencies; it turns convolution into multiplication, differentiation into multiplication by `iw`, and is its own kind of inverse (see [`inverse_transform`](inverse_transform.md)). Other variables of the input stay free, so a family of functions transforms into a family of transforms.

Three conventions differ only in where the factors of `2π` go:

| `Convention` | Forward `F` | Inverse prefactor |
|---|---|---|
| `Angular` (default) | `∫ f(x) e^{−iwx} dx` | `1/(2π)` |
| `Unitary` | `(2π)^{−1/2} ∫ f(x) e^{−iwx} dx` | `(2π)^{−1/2}` |
| `Ordinary` | `∫ f(x) e^{−2πiξx} dx` (frequency in cycles) | `1` |

When the input is built from exponentials and polynomials (including Gaussians) the integral has a closed form and the result is exact (`Structural`). Otherwise the result is an *unresolved integral over the infinite range*, evaluated numerically when values arrive (`Quadrature`); it still differentiates, composes and saves like any function, and `d/dw` of the transform is the transform of the differentiated integrand. The frequency name must differ from the integration variable and from every other variable already in the function. The input must be built from algebra on its variables (not opaque callables).

## Parameters

`std::string_view variable`
: The variable to integrate out.

`std::string_view frequency`
: The name of the new frequency variable. Must differ from `variable` and from every variable of the function.

`operators::fourier::Convention convention`
: Where the `2π` factors go: `Angular` (default), `Unitary` or `Ordinary`.

## Returns

An `Operator` whose application returns the transform as a `Function` of the frequency (and the function's other variables).

## Errors

- `the integration and frequency variables must be distinct, non-empty names`.
- An unknown variable raises `unknown variable '<name>'`.
- A numerical transform that does not converge raises `unresolved integral quadrature did not converge`.

## Practical Examples

### Example 1

The three conventions on a Gaussian. `exp(−x²)` has transform `√π exp(−w²/4)` (angular), `exp(−w²/4)/√2` (unitary) and `√π exp(−π²ξ²)` (ordinary). Each is exact (`Structural`) and the numbers match the formulas.

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

  const math::Function g = math::function({"x"}, [](math::Function x) { return (-(x * x)).exp(); });
  const double w = 0.7;
  const auto value = [&](fourier::Convention c) { return operators::fourier::transform("x", "w", c)(g)(w).item<std::complex<double>>().real(); };
  out.writeln("Angular:  ", value(fourier::Convention::Angular), "  sqrt(pi) e^(-w^2/4)       =", std::sqrt(pi) * std::exp(-w * w / 4.0));
  out.writeln("Unitary:  ", value(fourier::Convention::Unitary), "  e^(-w^2/4) / sqrt 2       =", std::exp(-w * w / 4.0) / std::sqrt(2.0));
  out.writeln("Ordinary: ", value(fourier::Convention::Ordinary), "  sqrt(pi) e^(-pi^2 xi^2)   =", std::sqrt(pi) * std::exp(-pi * pi * w * w));
}
```

Output

```text
Angular:   1.56810   sqrt(pi) e^(-w^2/4)       = 1.56810
Unitary:   0.62558   e^(-w^2/4) / sqrt 2       = 0.62558
Ordinary:  0.01407   sqrt(pi) e^(-pi^2 xi^2)   = 0.01407
```

### Example 2

A parameter stays free. The transform of `exp(−a x²)` is `√(π/a) exp(−w²/(4a))`: a function of the frequency `w` and of the parameter `a`, evaluated here for three values of `a` at one frequency. The parameter comes first in the variable list (`a`, then the new `w`), so the call names its arguments.

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

  const math::Function g = math::function({"x", "a"}, [](math::Function x, math::Function a) { return (-(a * x * x)).exp(); });
  const math::Function F = operators::fourier::transform("x", "w")(g);
  out.writeln("variables:", F.variables()[0], F.variables()[1]);
  const auto a = pluss::make_series({0.5, 1.0, 2.0});
  const auto w = pluss::make_series({1.0, 1.0, 1.0});
  out.writeln("F(w = 1, a):", F({{"w", w}, {"a", a}}).real().items<double>());
  out.writeln("sqrt(pi/a) e^(-1/(4a)):", (pi / a).sqrt().mul((-(1.0 / (4.0 * a))).exp()).items<double>());
}
```

Output

```text
variables: a w
F(w = 1, a): [1.52035, 1.38039, 1.10605]
sqrt(pi/a) e^(-1/(4a)): [1.52035, 1.38039, 1.10605]
```

### Example 3

When there is no closed form the transform is numerical. `sech(x) = 1/cosh(x)` has transform `π sech(πw/2)`; the value at `w = 1` agrees to the printed digits, and `provenance()` reports `Quadrature`.

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

  const math::Function g = math::function({"x"}, [](math::Function x) { return 1.0 / x.cosh(); });
  const math::Function F = operators::fourier::transform("x", "w")(g);
  out.writeln("F(1) =", F(1.0).item<std::complex<double>>().real(), "   pi sech(pi/2) =", pi / std::cosh(pi / 2.0));
  out.writeln("provenance is Quadrature:", F.provenance() == math::Provenance::Quadrature);
}
```

Output

```text
F(1) = 1.25204    pi sech(pi/2) = 1.25204
provenance is Quadrature: true
```

### Example 4

The derivative of a transform. For `g = exp(−x²)`, `F(w) = √π e^{−w²/4}` has `F′(w) = −(w/2) F(w)`, which `differential` produces exactly.

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
  const double pi = std::acos(-1.0);

  const math::Function g = math::function({"x"}, [](math::Function x) { return (-(x * x)).exp(); });
  const math::Function F = operators::fourier::transform("x", "w")(g);
  const math::Function dF = operators::differential("w")(F);
  const double w = 0.7;
  out.writeln("F'(0.7)        =", dF(w).item<std::complex<double>>().real());
  out.writeln("-(w/2) F(0.7)  =", -(w / 2.0) * F(w).item<std::complex<double>>().real());
}
```

Output

```text
F'(0.7)        = -0.54884
-(w/2) F(0.7)  = -0.54884
```

### Example 5

Clashing or unknown variable names are refused: the frequency must differ from the integration variable, and the integration variable must be one of the function's.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace fourier = pluss::operators::fourier;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto first_line = [](const pluss::Error &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  const math::Function g = math::function({"x"}, [](math::Function x) { return (-(x * x)).exp(); });
  try { (void)operators::fourier::transform("x", "x")(g); } catch (const pluss::Error &e) { out.writeln("same name:       ", first_line(e)); }
  try { (void)operators::fourier::transform("z", "w")(g); } catch (const pluss::Error &e) { out.writeln("unknown variable: ", first_line(e)); }
}
```

Output

```text
same name:        operators: the integration and frequency variables must be distinct, non-empty names
unknown variable:  math::Function: unknown variable 'z'
```

## See Also

- [`inverse_transform`](inverse_transform.md): the inverse
- [`cosine_transform`](cosine_transform.md): for even functions on the half line
- [`laplace::transform`](../laplace/transform.md): the one-sided cousin

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sine_transform.html">
    <span class="label">Previous</span>
    <span class="title">sine_transform</span>
  </a>
</div>
<!-- apiref:nav:end -->
