<!-- generated from sin.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 96685ae91fa8 -->

# `math::Function::sin`

[Back to `math::Function`](../Function.md)

The sine of a function (in radians), as a new function.

## Signature

```cpp
math::Function math::Function::sin() const;
```

## What It Does

`f.sin()` builds `x ↦ sin f(x)`, with the argument in radians. It is defined for every real and complex argument and never raises a domain error. Its derivative is `(sin u)′ = u′ cos u`.

Sine and [`cos`](cos.md) are linked by `sin² + cos² = 1` and by each being the derivative of the other up to sign; differentiating `sin` twice returns `−sin`, which is the equation of simple harmonic motion. Built functions can use that: a Taylor series, a Fourier coefficient and a transform of a sine are all exact because the rules are exact.

## Returns

A new `Function` whose value is `sin` of the receiver's value, with the same variables and output shape.

## Errors

- The function must be defined.

## Practical Examples

### Example 1

`sin` evaluated on a small grid, next to the standard-library value. Each element is computed independently, so the result has the shape of the input.

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
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.sin(); });
  const auto x = pluss::make_series({0.0, 0.5, 1.5707963267948966, 3.0});
  out.writeln("x:           ", x.items<double>());
  out.writeln("f(x):        ", f({x}).items<double>());
  out.writeln("std::sin(x):", pluss::make_series({std::sin((0.0)), std::sin((0.5)), std::sin((1.5707963267948966)), std::sin((3.0))}).items<double>());
}
```

Output

```text
x:            [0.0000, 0.5000, 1.5708, 3.0000]
f(x):         [0.0000, 0.4794, 1.0000, 0.1411]
std::sin(x): [0.0000, 0.4794, 1.0000, 0.1411]
```

The two rows agree to the printed digits.

### Example 2

The derivative rule. Differentiating `sin` with `operators::differential` gives the exact formula, `(sin x)′ = cos x`.

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
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.sin(); });
  const math::Function df = operators::differential("x")(f);
  const auto x = pluss::make_series({0.0, 0.5, 1.5707963267948966, 3.0});
  const auto exact = [](double v) { return std::cos(v); };
  out.writeln("f'(x):        ", df({x}).items<double>());
  out.writeln("closed form:  ", pluss::make_series({exact(0.0), exact(0.5), exact(1.5707963267948966), exact(3.0)}).items<double>());
}
```

Output

```text
f'(x):         [1.0000, 0.8776, 0.0000, -0.9900]
closed form:   [1.0000, 0.8776, 0.0000, -0.9900]
```

### Example 3

Periodicity and parity: `sin(x + 2π) = sin x` and `sin(−x) = −sin x`.

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
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.sin(); });
  const math::Function shifted = math::function({"x"}, [](math::Function x) { return (x + 2.0 * std::acos(-1.0)).sin(); });
  const math::Function mirrored = math::function({"x"}, [](math::Function x) { return (-x).sin(); });
  const auto x = pluss::make_series({0.3, 1.0, 2.5});
  out.writeln("sin x:           ", f({x}).items<double>());
  out.writeln("sin(x + 2 pi):   ", shifted({x}).items<double>());
  out.writeln("sin(-x):         ", mirrored({x}).items<double>());
}
```

Output

```text
sin x:            [0.2955, 0.8415, 0.5985]
sin(x + 2 pi):    [0.2955, 0.8415, 0.5985]
sin(-x):          [-0.2955, -0.8415, -0.5985]
```

### Example 4

Simple harmonic motion: differentiating `sin(ωt)` twice gives `−ω² sin(ωt)`, so `y″ + ω² y = 0`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const double omega = 3.0;
  const math::Function y = math::function({"t"}, [&](math::Function t) { return (omega * t).sin(); });
  const math::Function ydd = operators::differential("t", 2)(y);
  const auto t = pluss::make_series({0.0, 0.4, 1.1});
  out.writeln("y'' + omega^2 y:", (ydd({t}) + omega * omega * y({t})).items<double>());
}
```

Output

```text
y'' + omega^2 y: [0.0000, 0.0000, 0.0000]
```

## See Also

- [`cos`](cos.md): the cosine
- [`tan`](tan.md): the tangent
- [`sinh`](sinh.md): the hyperbolic sine

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shape.html">
    <span class="label">Previous</span>
    <span class="title">shape</span>
  </a>
  <a class="next-link" href="sinh.html">
    <span class="label">Next</span>
    <span class="title">sinh</span>
  </a>
</div>
<!-- apiref:nav:end -->
