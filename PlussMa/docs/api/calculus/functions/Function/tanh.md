<!-- generated from tanh.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5887a8ba4407 -->

# `math::Function::tanh`

[Back to `math::Function`](../Function.md)

The hyperbolic tangent of a function, as a new function.

## Signature

```cpp
math::Function math::Function::tanh() const;
```

## What It Does

`f.tanh()` builds `x ↦ tanh f(x) = sinh / cosh`. It is defined for every argument, is odd, increases from −1 to 1, and saturates quickly: for `|x| ≳ 20` it equals ±1 to double precision. Its derivative is `(tanh u)′ = u′ (1 − tanh² u)`.

It is the standard smooth squashing function, used wherever a bounded, differentiable switch is needed.

## Returns

A new `Function` whose value is `tanh` of the receiver's value, with the same variables and output shape.

## Errors

- The function must be defined.

## Practical Examples

### Example 1

`tanh` evaluated on a small grid, next to the standard-library value. Each element is computed independently, so the result has the shape of the input.

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
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.tanh(); });
  const auto x = pluss::make_series({-3.0, -0.5, 0.0, 2.0});
  out.writeln("x:           ", x.items<double>());
  out.writeln("f(x):        ", f({x}).items<double>());
  out.writeln("std::tanh(x):", pluss::make_series({std::tanh((-3.0)), std::tanh((-0.5)), std::tanh((0.0)), std::tanh((2.0))}).items<double>());
}
```

Output

```text
x:            [-3.0000, -0.5000, 0.0000, 2.0000]
f(x):         [-0.9951, -0.4621, 0.0000, 0.9640]
std::tanh(x): [-0.9951, -0.4621, 0.0000, 0.9640]
```

The two rows agree to the printed digits.

### Example 2

The derivative rule. Differentiating `tanh` with `operators::differential` gives the exact formula, `(tanh x)′ = 1 − tanh² x`.

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
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.tanh(); });
  const math::Function df = operators::differential("x")(f);
  const auto x = pluss::make_series({-3.0, -0.5, 0.0, 2.0});
  const auto exact = [](double v) { return 1.0 - std::tanh(v) * std::tanh(v); };
  out.writeln("f'(x):        ", df({x}).items<double>());
  out.writeln("closed form:  ", pluss::make_series({exact(-3.0), exact(-0.5), exact(0.0), exact(2.0)}).items<double>());
}
```

Output

```text
f'(x):         [0.0099, 0.7864, 1.0000, 0.0707]
closed form:   [0.0099, 0.7864, 1.0000, 0.0707]
```

### Example 3

Saturation: the value is within 10⁻¹⁵ of ±1 well before the argument reaches 20, and the derivative vanishes there.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.tanh(); });
  const math::Function df = operators::differential("x")(f);
  const auto x = pluss::make_series({0.5, 2.0, 5.0, 20.0, 40.0});
  out.writeln("tanh:   ", f({x}).items<double>());
  out.writeln("tanh':  ", df({x}).items<double>());
}
```

Output

```text
tanh:    [0.4621171573, 0.9640275801, 0.9999092043, 1.0000000000, 1.0000000000]
tanh':   [0.7864477330, 0.0706508249, 0.0001815832, 0.0000000000, 0.0000000000]
```

### Example 4

Oddness, and `tanh = sinh / cosh`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function direct = math::function({"x"}, [](math::Function x) { return x.tanh(); });
  const math::Function ratio = math::function({"x"}, [](math::Function x) { return x.sinh() / x.cosh(); });
  const auto x = pluss::make_series({0.4, 1.3});
  out.writeln("tanh:       ", direct({x}).items<double>());
  out.writeln("sinh/cosh:  ", ratio({x}).items<double>());
  out.writeln("tanh(-x):   ", direct({-x}).items<double>());
}
```

Output

```text
tanh:        [0.3799, 0.8617]
sinh/cosh:   [0.3799, 0.8617]
tanh(-x):    [-0.3799, -0.8617]
```

## See Also

- [`sinh`](sinh.md): the hyperbolic sine
- [`cosh`](cosh.md): the hyperbolic cosine
- [`tan`](tan.md): the circular tangent

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tan.html">
    <span class="label">Previous</span>
    <span class="title">tan</span>
  </a>
  <a class="next-link" href="trace.html">
    <span class="label">Next</span>
    <span class="title">trace</span>
  </a>
</div>
<!-- apiref:nav:end -->
