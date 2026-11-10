<!-- generated from differential.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::differential`

The partial derivative of a function with respect to one variable, to any order.

## Signature

```cpp
operators::Operator operators::differential(std::string_view variable, std::int64_t order = 1, operators::DiffOptions options = {});
```

## What It Does

`differential("x")` is the operator `f ↦ ∂f/∂x`, and `differential("x", n)` is the `n`-th partial derivative. Applied to a function it returns a new function with the *same variables*: differentiating with respect to `x` leaves `y` in place, and a vector- or matrix-valued function is differentiated componentwise, keeping its shape.

The derivative is exact. Pluss applies the differentiation rules (sum, product, quotient, chain, and the derivative of every elementary function) to the recorded formula and returns a formula; nothing is approximated by differences. The cost of building the result grows with the order and the size of the formula, and the `max_nodes` limit in [`DiffOptions`](../operators/DiffOptions.md) refuses runaway growth. Evaluating the derivative afterwards is as cheap as evaluating any formula, and works on arrays of points.

**Mixed partial derivatives** are compositions: `compose(differential("y"), differential("x"))` is `∂²/∂x∂y`. **Non-smooth points** (the corner of `|x|`) follow the `nonsmooth` option, and **opaque functions** (callables on `Series`) follow the `fallback` option; see [`DiffOptions`](../operators/DiffOptions.md). For a complex variable use [`wirtinger`](wirtinger.md); for a vector of derivatives use [`gradient`](gradient.md), [`jacobian`](jacobian.md) or [`hessian`](hessian.md).

## Parameters

`std::string_view variable`
: The variable to differentiate with respect to. It must be a variable of the function the operator is applied to.

`std::int64_t order`
: How many times to differentiate. Positive; default 1.

`operators::DiffOptions options`
: Non-smooth policy, opaque-function fallback and the expression-size limit. See `DiffOptions`.

## Returns

An `Operator` whose application returns the derivative as a `Function` with the same variables and output shape.

## Errors

- An empty variable name or a non-positive order is refused when the operator is built.
- A variable the function lacks is refused when the operator is applied.
- A graph larger than `max_nodes` raises `transformed graph exceeds max_nodes safeguard`.
- An opaque function without `Fallback::Autodiff` raises `opaque Function requires explicit DiffOptions{.fallback = math::Fallback::Autodiff}`.

## Practical Examples

### Example 1

Partial derivatives of a function of two variables, each evaluated at a point. For `f = x² y + sin y`: `f_x = 2xy` and `f_y = x² + cos y`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x * x * y + y.sin(); });
  const auto point = {pluss::make_series(2.0), pluss::make_series(0.5)};
  out.writeln("f_x =", operators::differential("x")(f)(point).item<double>(), "  (2 x y = 2)");
  out.writeln("f_y =", operators::differential("y")(f)(point).item<double>(), "  (x^2 + cos y =", 4.0 + std::cos(0.5), ")");
}
```

Output

```text
f_x = 2.0000   (2 x y = 2)
f_y = 4.8776   (x^2 + cos y = 4.8776 )
```

### Example 2

Higher orders. The derivatives of `sin` cycle with period four: `cos`, `−sin`, `−cos`, `sin`. Order 5 is the same as order 1.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function s = math::function({"x"}, [](math::Function x) { return x.sin(); });
  const double x = 0.5;
  for (const std::int64_t n : {1, 2, 3, 4, 5}) {
    out.writeln("order", n, ":", operators::differential("x", n)(s)(x).item<double>());
  }
  out.writeln("cos, -sin, -cos, sin, cos at 0.5:", std::cos(x), -std::sin(x), -std::cos(x), std::sin(x), std::cos(x));
}
```

Output

```text
order 1 : 0.8776
order 2 : -0.4794
order 3 : -0.8776
order 4 : 0.4794
order 5 : 0.8776
cos, -sin, -cos, sin, cos at 0.5: 0.8776 -0.4794 -0.8776 0.4794 0.8776
```

### Example 3

Mixed partial derivatives by composition. `∂²f/∂x∂y` can be taken in either order, and the two agree (Schwarz). For `f = x³y²` the mixed partial is `6x²y`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x * x * x * y * y; });
  const operators::Operator Dx = operators::differential("x"), Dy = operators::differential("y");
  const auto point = {pluss::make_series(2.0), pluss::make_series(3.0)};
  out.writeln("d/dy d/dx f =", operators::compose(Dy, Dx)(f)(point).item<double>());
  out.writeln("d/dx d/dy f =", operators::compose(Dx, Dy)(f)(point).item<double>());
  out.writeln("6 x^2 y     =", 6.0 * 4.0 * 3.0);
}
```

Output

```text
d/dy d/dx f = 72.0000
d/dx d/dy f = 72.0000
6 x^2 y     = 72.0000
```

### Example 4

A vector-valued function is differentiated componentwise and keeps its shape. The curve `(cos t, sin t, t)` has tangent `(−sin t, cos t, 1)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function curve = math::function({"t"}, [](math::Function t) { return math::stack({t.cos(), t.sin(), t}); });
  const math::Function tangent = operators::differential("t")(curve);
  out.writeln("shape of the tangent:", tangent.shape());
  out.writeln("tangent at t = 1:  ", tangent(1.0).items<double>());
  out.writeln("(-sin, cos, 1):    ", pluss::make_series({-std::sin(1.0), std::cos(1.0), 1.0}).items<double>());
}
```

Output

```text
shape of the tangent: [3]
tangent at t = 1:   [-0.8415, 0.5403, 1.0000]
(-sin, cos, 1):     [-0.8415, 0.5403, 1.0000]
```

### Example 5

A derivative is a function, so it evaluates on arrays of points in one call. Here the derivative of `x exp(−x²)` on a grid of five points.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x * (-(x * x)).exp(); });
  const math::Function df = operators::differential("x")(f);
  const auto x = pluss::make_series({-1.0, -0.5, 0.0, 0.5, 1.0});
  out.writeln("x:       ", x.items<double>());
  out.writeln("f'(x):   ", df({x}).items<double>());
  out.writeln("(1-2x^2)e^(-x^2):", ((1.0 - 2.0 * x * x) * (-(x * x)).exp()).items<double>());
}
```

Output

```text
x:        [-1.0000, -0.5000, 0.0000, 0.5000, 1.0000]
f'(x):    [-0.3679, 0.3894, 1.0000, 0.3894, -0.3679]
(1-2x^2)e^(-x^2): [-0.3679, 0.3894, 1.0000, 0.3894, -0.3679]
```

### Example 6

The corner of the absolute value. `NonSmooth` chooses what the derivative is at 0: `Piecewise` gives `NaN`, `Subgradient` gives 0. (`Strict`, the default, refuses to evaluate there.)

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

static const char *name(math::NonSmooth n) {
  switch (n) {
  case math::NonSmooth::Strict: return "Strict";
  case math::NonSmooth::Piecewise: return "Piecewise";
  case math::NonSmooth::Subgradient: return "Subgradient";
  case math::NonSmooth::Autodiff: return "Autodiff";
  }
  return "?";
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.abs(); });
  const auto x = pluss::make_series({-2.0, 0.0, 2.0});
  for (const math::NonSmooth policy : {math::NonSmooth::Piecewise, math::NonSmooth::Subgradient}) {
    out.writeln(name(policy), operators::differential("x", 1, {.nonsmooth = policy})(f)({x}).items<double>());
  }
}
```

Output

```text
Piecewise [-1.0000, nan, 1.0000]
Subgradient [-1.0000, 0.0000, 1.0000]
```

### Example 7

An opaque function (a callable on `Series`) needs `Fallback::Autodiff`; the derivative is then computed by automatic differentiation and the function's provenance says so.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto first_line = [](const pluss::Error &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  const math::Function opaque = math::function({"x"}, [](const pluss::Series &x) { return x * x * x; });
  try { (void)operators::differential("x")(opaque); } catch (const pluss::Error &e) { out.writeln("default:", first_line(e)); }
  const math::Function d2 = operators::differential("x", 2, {.fallback = math::Fallback::Autodiff})(opaque);
  out.writeln("f''(2) =", d2(2.0).item<double>(), "  (6 x = 12)   provenance Autodiff:", d2.provenance() == math::Provenance::Autodiff);
}
```

Output

```text
default: operators::differential: opaque Function requires explicit DiffOptions{.fallback = math::Fallback::Autodiff}
f''(2) = 12.0000   (6 x = 12)   provenance Autodiff: true
```

### Example 8

Mistakes are refused with a message that names them: an unknown variable, a non-positive order, an empty name and an undefined input.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto first_line = [](const pluss::Error &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  const math::Function f = math::function({"x"}, [](math::Function x) { return x; });
  try { (void)operators::differential("z")(f); } catch (const pluss::Error &e) { out.writeln("unknown variable:", first_line(e)); }
  try { (void)operators::differential("x", 0); } catch (const pluss::Error &e) { out.writeln("order zero:      ", first_line(e)); }
  try { (void)operators::differential("", 1); } catch (const pluss::Error &e) { out.writeln("empty name:      ", first_line(e)); }
  try { (void)operators::differential("x")(math::Function{}); } catch (const pluss::Error &e) { out.writeln("undefined input:  ", first_line(e)); }
}
```

Output

```text
unknown variable: operators::differential: unknown variable 'z'
order zero:       operators::differential: order must be positive
empty name:       operators::differential: variable must not be empty
undefined input:   operators::Operator: input Function must be defined
```

## See Also

- [`gradient`](gradient.md): all first partials of a scalar function
- [`DiffOptions`](../operators/DiffOptions.md): non-smooth and opaque policies
- [`wirtinger`](wirtinger.md): derivatives with respect to a complex variable

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="covariant.html">
    <span class="label">Previous</span>
    <span class="title">covariant</span>
  </a>
  <a class="next-link" href="gradient.html">
    <span class="label">Next</span>
    <span class="title">gradient</span>
  </a>
</div>
<!-- apiref:nav:end -->
