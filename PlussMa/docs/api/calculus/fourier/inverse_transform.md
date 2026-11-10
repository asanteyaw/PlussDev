<!-- generated from inverse_transform.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::fourier::inverse_transform`

The inverse Fourier transform: recovers a function of the original variable from its transform.

## Signature

```cpp
operators::Operator operators::fourier::inverse_transform(std::string_view frequency, std::string_view variable, operators::fourier::Convention convention = operators::fourier::Convention::Angular);
```

## What It Does

`inverse_transform("w", "x")` acts on a function of the frequency `w` and returns a function of a new variable `x`: `f(x) = (1/2π) ∫ F(w) e^{iwx} dw` in the angular convention (the prefactor follows the [`Convention`](transform.md): `1/2π`, `1/√2π`, or `1`). The convention must match the one used to produce `F`; a mismatch changes the result by the ratio of the prefactors.

The result is an integral over the whole frequency line, so it is evaluated numerically when values arrive (`Quadrature`); for the inverse of a transform that is itself an unresolved integral, the integrals nest and may not converge, so build the transform from a formula. The new variable name must not clash with a variable already in the function's history, *including one that was integrated out*: a transform of `g(x)` has already used `x`, so invert it into a different name, such as `"y"`. The inverse of a real, even transform is real; numerically the result is complex with a negligible imaginary part.

## Parameters

`std::string_view frequency`
: The variable of the input function (the frequency).

`std::string_view variable`
: The name of the new variable. Must not clash with any variable already in the function's history.

`operators::fourier::Convention convention`
: Must match the convention of the forward transform; default `Angular`.

## Returns

An `Operator` whose application returns the inverse transform as a `Function` of `variable`.

## Errors

- `variable 'x' already exists` when the new name clashes with an existing or integrated-out variable.
- `the integration and frequency variables must be distinct, non-empty names`.

## Practical Examples

### Example 1

A round trip. The transform of `exp(−x²)` is a function of `w`; inverting it into `y` gives back `exp(−y²)`. (Into the name `x` would be refused: `x` was used by the forward transform.)

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
  const math::Function g = math::function({"x"}, [](math::Function x) { return (-(x * x)).exp(); });
  const math::Function F = operators::fourier::transform("x", "w")(g);
  const math::Function back = operators::fourier::inverse_transform("w", "y")(F);
  for (const double y : {0.0, 0.5, 1.5}) {
    out.writeln("y =", y, ":", back(y).item<std::complex<double>>().real(), "  exp(-y^2) =", std::exp(-y * y));
  }
}
```

Output

```text
y = 0.00000 : 1.00000   exp(-y^2) = 1.00000
y = 0.50000 : 0.77880   exp(-y^2) = 0.77880
y = 1.50000 : 0.10540   exp(-y^2) = 0.10540
```

### Example 2

Inverting a formula. `F(w) = √π exp(−w²/4)` is the transform of `exp(−x²)`; built directly as a function of `w`, its inverse is the Gaussian again.

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

  const math::Function F = math::function({"w"}, [&](math::Function w) { return std::sqrt(pi) * (-(w * w) / 4.0).exp(); });
  const math::Function f = operators::fourier::inverse_transform("w", "x")(F);
  out.writeln("f(0.8) =", f(0.8).item<std::complex<double>>().real(), "  exp(-0.64) =", std::exp(-0.64));
}
```

Output

```text
f(0.8) = 0.52729   exp(-0.64) = 0.52729
```

### Example 3

The convention must match. A unitary forward transform inverted with the angular prefactor is off by the factor `1/√(2π)`; inverted with the unitary convention it recovers the function. Same point, both inverses.

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
  const math::Function F = operators::fourier::transform("x", "w", fourier::Convention::Unitary)(g);
  const auto real_at = [](const math::Function &f, double y) { return f(y).item<std::complex<double>>().real(); };
  const double matched = real_at(operators::fourier::inverse_transform("w", "y", fourier::Convention::Unitary)(F), 0.5);
  const double mismatched = real_at(operators::fourier::inverse_transform("w", "y", fourier::Convention::Angular)(F), 0.5);
  out.writeln("exp(-0.25)             =", std::exp(-0.25));
  out.writeln("unitary inverse        =", matched);
  out.writeln("angular inverse        =", mismatched, "  ratio to exact:", mismatched / std::exp(-0.25), " (1/sqrt(2 pi) =", 1.0 / std::sqrt(2.0 * pi), ")");
}
```

Output

```text
exp(-0.25)             = 0.77880
unitary inverse        = 0.77880
angular inverse        = 0.31070   ratio to exact: 0.39894  (1/sqrt(2 pi) = 0.39894 )
```

### Example 4

The new variable must not already appear in the function's history. Inverting the transform of a function of `x` back into `x` is refused with a message that names the variable.

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
  const math::Function F = operators::fourier::transform("x", "w")(g);
  try { (void)operators::fourier::inverse_transform("w", "x")(F); } catch (const pluss::Error &e) { out.writeln(first_line(e)); }
}
```

Output

```text
math::Function: variable 'x' already exists
```

## See Also

- [`transform`](transform.md): the forward transform and its conventions
- [`laplace::inverse_transform`](../laplace/inverse_transform.md): the Laplace inverse

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cosine_transform.html">
    <span class="label">Previous</span>
    <span class="title">cosine_transform</span>
  </a>
  <a class="next-link" href="series.html">
    <span class="label">Next</span>
    <span class="title">series</span>
  </a>
</div>
<!-- apiref:nav:end -->
