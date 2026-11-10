<!-- generated from tan.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d5ce786bcf3c -->

# `math::Function::tan`

[Back to `math::Function`](../Function.md)

The tangent of a function (in radians), as a new function.

## Signature

```cpp
math::Function math::Function::tan() const;
```

## What It Does

`f.tan()` builds `x ↦ tan f(x) = sin f(x) / cos f(x)`. It is defined for every argument the floating-point arithmetic can represent, but it has poles at odd multiples of π/2, where it is unbounded; near a pole the value is a very large number of either sign, not an error. Its derivative is `(tan u)′ = u′ (1 + tan² u) = u′ / cos² u`.

Use it where a slope or ratio of components is the quantity: angles of a line, the tangent of a phase.

## Returns

A new `Function` whose value is `tan` of the receiver's value, with the same variables and output shape.

## Errors

- The function must be defined.

## Practical Examples

### Example 1

`tan` evaluated on a small grid, next to the standard-library value. Each element is computed independently, so the result has the shape of the input.

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
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.tan(); });
  const auto x = pluss::make_series({-1.0, 0.0, 0.5, 1.0});
  out.writeln("x:           ", x.items<double>());
  out.writeln("f(x):        ", f({x}).items<double>());
  out.writeln("std::tan(x):", pluss::make_series({std::tan((-1.0)), std::tan((0.0)), std::tan((0.5)), std::tan((1.0))}).items<double>());
}
```

Output

```text
x:            [-1.0000, 0.0000, 0.5000, 1.0000]
f(x):         [-1.5574, 0.0000, 0.5463, 1.5574]
std::tan(x): [-1.5574, 0.0000, 0.5463, 1.5574]
```

The two rows agree to the printed digits.

### Example 2

The derivative rule. Differentiating `tan` with `operators::differential` gives the exact formula, `(tan x)′ = 1 + tan² x`.

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
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.tan(); });
  const math::Function df = operators::differential("x")(f);
  const auto x = pluss::make_series({-1.0, 0.0, 0.5, 1.0});
  const auto exact = [](double v) { return 1.0 + std::tan(v) * std::tan(v); };
  out.writeln("f'(x):        ", df({x}).items<double>());
  out.writeln("closed form:  ", pluss::make_series({exact(-1.0), exact(0.0), exact(0.5), exact(1.0)}).items<double>());
}
```

Output

```text
f'(x):         [3.4255, 1.0000, 1.2984, 3.4255]
closed form:   [3.4255, 1.0000, 1.2984, 3.4255]
```

### Example 3

Poles. Approaching π/2 from either side the value grows without bound and changes sign; at the grid point nearest π/2 it is of the order 10¹⁶.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(1));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.tan(); });
  const double half_pi = std::acos(-1.0) / 2.0;
  out.writeln("tan(pi/2 - 0.01):", f(half_pi - 0.01).item<double>());
  out.writeln("tan(pi/2 + 0.01):", f(half_pi + 0.01).item<double>());
  out.writeln("tan(pi/2):       ", f(half_pi).item<double>());
}
```

Output

```text
tan(pi/2 - 0.01): 100.0
tan(pi/2 + 0.01): -100.0
tan(pi/2):        16331239353195370.0
```

### Example 4

`tan x = sin x / cos x`: the two ways agree away from the poles.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const math::Function direct = math::function({"x"}, [](math::Function x) { return x.tan(); });
  const math::Function ratio = math::function({"x"}, [](math::Function x) { return x.sin() / x.cos(); });
  const auto x = pluss::make_series({-1.2, 0.3, 0.9});
  out.writeln("tan:       ", direct({x}).items<double>());
  out.writeln("sin / cos: ", ratio({x}).items<double>());
}
```

Output

```text
tan:        [-2.5721516221, 0.3093362496, 1.2601582176]
sin / cos:  [-2.5721516221, 0.3093362496, 1.2601582176]
```

## See Also

- [`sin`](sin.md): the sine
- [`cos`](cos.md): the cosine
- [`tanh`](tanh.md): the hyperbolic tangent

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sub.html">
    <span class="label">Previous</span>
    <span class="title">sub</span>
  </a>
  <a class="next-link" href="tanh.html">
    <span class="label">Next</span>
    <span class="title">tanh</span>
  </a>
</div>
<!-- apiref:nav:end -->
