<!-- generated from inverse_transform.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::laplace::inverse_transform`

The inverse Laplace transform by Talbot's method: an explicit formula in `t`, with no quadrature at evaluation.

## Signature

```cpp
operators::Operator operators::laplace::inverse_transform(std::string_view frequency, std::string_view variable, std::int64_t terms = 32);
```

## What It Does

`laplace::inverse_transform("s", "t")` acts on a function `F(s)` and returns `f(t)`, a function of the new variable `t > 0`. It uses Talbot's contour method on Weideman's optimised contour: `f(t)` is approximated by a sum of `terms/2` evaluations of `F` at complex points that depend on `t`. The result is an ordinary function of `t` built from `F` itself, with no integral to evaluate, so it is fast and differentiable.

The accuracy is about `1.3^{−terms}`: 32 terms (the default) give roughly ten digits when the singularities of `F` lie in the left half plane at a distance from the imaginary axis that grows with `t`. Poles on the imaginary axis (undamped oscillations) lose accuracy for large `t`, which more terms postpone. `terms` must be even and lie in `[8, 256]`. `F` must be built from algebra on `s` (no integrals or opaque callables). The new variable name must not clash with an existing variable of `F`.

## Parameters

`std::string_view frequency`
: The variable of the input function (the frequency `s`).

`std::string_view variable`
: The name of the new time variable; must not clash with an existing variable.

`std::int64_t terms`
: The number of evaluation points; even, in `[8, 256]`; default 32.

## Returns

An `Operator` whose application returns `f(t)` as a `Function` of the time variable.

## Errors

- `terms must be even and lie in [8, 256]`.
- A clashing variable name is refused.

## Practical Examples

### Example 1

`1/(s + 1)²` inverts to `t e^{−t}`. At `t = 2` that is `0.27067`; the default 32 terms reproduce it to the printed digits.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace laplace = pluss::operators::laplace;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function F = math::function({"s"}, [](math::Function s) { return 1.0 / ((s + 1.0) * (s + 1.0)); });
  const math::Function f = operators::laplace::inverse_transform("s", "t")(F);
  out.writeln("f(2) =", f(2.0).item<double>(), "   t e^-t =", 2.0 * std::exp(-2.0));
  out.writeln("f at t = 0.5, 1, 4:", f({pluss::make_series({0.5, 1.0, 4.0})}).items<double>());
}
```

Output

```text
f(2) = 0.270671    t e^-t = 0.270671
f at t = 0.5, 1, 4: [0.303265, 0.367879, 0.073263]
```

### Example 2

Accuracy against the number of terms. With 8 terms the error is about `10⁻⁴`, with 16 terms about `10⁻⁹`, and with 32 terms it is below `10⁻¹²`; beyond that rounding dominates, so 64 terms is no better.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace laplace = pluss::operators::laplace;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(12));
  const math::Function F = math::function({"s"}, [](math::Function s) { return 1.0 / ((s + 1.0) * (s + 1.0)); });
  const double exact = 2.0 * std::exp(-2.0);
  for (const std::int64_t terms : {8, 16, 32, 64}) {
    const double value = operators::laplace::inverse_transform("s", "t", terms)(F)(2.0).item<double>();
    out.writeln("terms", terms, ": error", std::abs(value - exact));
  }
}
```

Output

```text
terms 8 : error 0.000132329178
terms 16 : error 0.000000003232
terms 32 : error 0.000000000000
terms 64 : error 0.000000000002
```

### Example 3

A round trip. The transform of `t e^{−t}` inverted into a different name recovers the function. Oscillations: `1/(s² + 1)` inverts to `sin t`, accurate for moderate `t`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace laplace = pluss::operators::laplace;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const math::Function g = math::function({"t"}, [](math::Function t) { return t * (-t).exp(); });
  const math::Function G = operators::laplace::transform("t", "s")(g);
  const math::Function back = operators::laplace::inverse_transform("s", "u")(G);
  out.writeln("t e^-t at 1.5, back:", g(1.5).item<double>(), back(1.5).item<double>());

  const math::Function F = math::function({"s"}, [](math::Function s) { return 1.0 / (s * s + 1.0); });
  const math::Function wave = operators::laplace::inverse_transform("s", "t")(F);
  out.writeln("1/(s^2+1) -> sin at t = 1, 3:", wave({pluss::make_series({1.0, 3.0})}).items<double>(), "  sin:", pluss::make_series({1.0, 3.0}).sin().items<double>());
}
```

Output

```text
t e^-t at 1.5, back: 0.33469524 0.33469524
1/(s^2+1) -> sin at t = 1, 3: [0.84147098, 0.14112001]   sin: [0.84147098, 0.14112001]
```

### Example 4

The checks: `terms` must be even and in `[8, 256]`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace laplace = pluss::operators::laplace;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto first_line = [](const pluss::Error &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  for (const std::int64_t terms : {3, 33, 300}) {
    try { (void)operators::laplace::inverse_transform("s", "t", terms); } catch (const pluss::Error &e) { out.writeln("terms =", terms, ":", first_line(e)); }
  }
}
```

Output

```text
terms = 3 : operators::laplace::inverse_transform: terms must be even and lie in [8, 256]
terms = 33 : operators::laplace::inverse_transform: terms must be even and lie in [8, 256]
terms = 300 : operators::laplace::inverse_transform: terms must be even and lie in [8, 256]
```

## See Also

- [`transform`](transform.md): the forward transform
- [`fourier::inverse_transform`](../fourier/inverse_transform.md): the Fourier inverse

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="transform.html">
    <span class="label">Next</span>
    <span class="title">transform</span>
  </a>
</div>
<!-- apiref:nav:end -->
