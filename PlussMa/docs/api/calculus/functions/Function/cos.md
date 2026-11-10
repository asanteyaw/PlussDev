<!-- generated from cos.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d06c3d5a468f -->

# `math::Function::cos`

[Back to `math::Function`](../Function.md)

The cosine of a function (in radians), as a new function.

## Signature

```cpp
math::Function math::Function::cos() const;
```

## What It Does

`f.cos()` builds `x ↦ cos f(x)`, with the argument in radians. It is defined for every real and complex argument. Its derivative is `(cos u)′ = −u′ sin u`.

Cosine is even, `cos(−x) = cos x`, which makes it the building block of cosine series and of the cosine transform. It is the real part of `e^{ix}`, and `cos² x = (1 + cos 2x)/2` links it to double angles.

## Returns

A new `Function` whose value is `cos` of the receiver's value, with the same variables and output shape.

## Errors

- The function must be defined.

## Practical Examples

### Example 1

`cos` evaluated on a small grid, next to the standard-library value. Each element is computed independently, so the result has the shape of the input.

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
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.cos(); });
  const auto x = pluss::make_series({0.5, 1.0, 2.0, 5.0});
  out.writeln("x:           ", x.items<double>());
  out.writeln("f(x):        ", f({x}).items<double>());
  out.writeln("std::cos(x):", pluss::make_series({std::cos((0.5)), std::cos((1.0)), std::cos((2.0)), std::cos((5.0))}).items<double>());
}
```

Output

```text
x:            [0.5000, 1.0000, 2.0000, 5.0000]
f(x):         [0.8776, 0.5403, -0.4161, 0.2837]
std::cos(x): [0.8776, 0.5403, -0.4161, 0.2837]
```

The two rows agree to the printed digits.

### Example 2

The derivative rule. Differentiating `cos` with `operators::differential` gives the exact formula, `(cos x)′ = −sin x`.

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
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.cos(); });
  const math::Function df = operators::differential("x")(f);
  const auto x = pluss::make_series({0.5, 1.0, 2.0, 5.0});
  const auto exact = [](double v) { return -std::sin(v); };
  out.writeln("f'(x):        ", df({x}).items<double>());
  out.writeln("closed form:  ", pluss::make_series({exact(0.5), exact(1.0), exact(2.0), exact(5.0)}).items<double>());
}
```

Output

```text
f'(x):         [-0.4794, -0.8415, -0.9093, 0.9589]
closed form:   [-0.4794, -0.8415, -0.9093, 0.9589]
```

### Example 3

The double-angle identity `cos 2x = 1 − 2 sin² x`, built as two functions in one trace and compared at three points.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(12));
  const math::Function difference = math::function({"x"}, [](math::Function x) { return (2.0 * x).cos() - (1.0 - 2.0 * x.sin() * x.sin()); });
  out.writeln("cos 2x - (1 - 2 sin^2 x):", difference({pluss::make_series({0.1, 1.0, 2.7})}).items<double>());
}
```

Output

```text
cos 2x - (1 - 2 sin^2 x): [0.000000000000, -0.000000000000, 0.000000000000]
```

### Example 4

Evenness: `cos(−x) = cos x`, so the derivative is odd: `cos′(−x) = −cos′(x)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.cos(); });
  const math::Function df = operators::differential("x")(f);
  out.writeln("cos(+1), cos(-1):   ", f(1.0).item<double>(), f(-1.0).item<double>());
  out.writeln("cos'(+1), cos'(-1): ", df(1.0).item<double>(), df(-1.0).item<double>());
}
```

Output

```text
cos(+1), cos(-1):    0.5403 0.5403
cos'(+1), cos'(-1):  -0.8415 0.8415
```

## See Also

- [`sin`](sin.md): the sine
- [`tan`](tan.md): the tangent
- [`cosh`](cosh.md): the hyperbolic cosine

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="contract.html">
    <span class="label">Previous</span>
    <span class="title">contract</span>
  </a>
  <a class="next-link" href="cosh.html">
    <span class="label">Next</span>
    <span class="title">cosh</span>
  </a>
</div>
<!-- apiref:nav:end -->
