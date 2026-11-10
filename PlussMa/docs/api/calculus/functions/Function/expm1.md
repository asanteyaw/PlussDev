<!-- generated from expm1.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 07ec5229834c -->

# `math::Function::expm1`

[Back to `math::Function`](../Function.md)

`eˣ − 1` of a function, computed accurately when `x` is small.

## Signature

```cpp
math::Function math::Function::expm1() const;
```

## What It Does

`f.expm1()` builds `x ↦ exp(f(x)) − 1`. For small `x`, `exp(x)` is very close to 1 and subtracting 1 cancels most of the correct digits; `expm1` avoids the cancellation. It is the right form for small rates over short times, such as a daily return `e^{r dt} − 1`.

It is defined for every real and complex argument and has no domain error. Its derivative is `(e^u − 1)′ = u′ e^u`. It is the inverse of [`log1p`](log1p.md): `log1p(expm1(x)) = x`.

## Returns

A new `Function` whose value is `expm1` of the receiver's value, with the same variables and output shape.

## Errors

- The function must be defined.

## Practical Examples

### Example 1

`expm1` evaluated on a small grid, next to the standard-library value. Each element is computed independently, so the result has the shape of the input.

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
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.expm1(); });
  const auto x = pluss::make_series({-2.0, 0.0, 0.5, 2.0});
  out.writeln("x:           ", x.items<double>());
  out.writeln("f(x):        ", f({x}).items<double>());
  out.writeln("std::expm1(x):", pluss::make_series({std::expm1((-2.0)), std::expm1((0.0)), std::expm1((0.5)), std::expm1((2.0))}).items<double>());
}
```

Output

```text
x:            [-2.0000, 0.0000, 0.5000, 2.0000]
f(x):         [-0.8647, 0.0000, 0.6487, 6.3891]
std::expm1(x): [-0.8647, 0.0000, 0.6487, 6.3891]
```

The two rows agree to the printed digits.

### Example 2

The derivative rule. Differentiating `expm1` with `operators::differential` gives the exact formula, `(eˣ − 1)′ = eˣ`.

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
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.expm1(); });
  const math::Function df = operators::differential("x")(f);
  const auto x = pluss::make_series({-2.0, 0.0, 0.5, 2.0});
  const auto exact = [](double v) { return std::exp(v); };
  out.writeln("f'(x):        ", df({x}).items<double>());
  out.writeln("closed form:  ", pluss::make_series({exact(-2.0), exact(0.0), exact(0.5), exact(2.0)}).items<double>());
}
```

Output

```text
f'(x):         [0.1353, 1.0000, 1.6487, 7.3891]
closed form:   [0.1353, 1.0000, 1.6487, 7.3891]
```

### Example 3

Accuracy for small `x`: at `x = 10⁻¹⁰` the difference `exp(x) − 1` has lost about six digits, while `expm1` keeps them. Printed to 18 decimals against the exact `x + x²/2`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(18));
  const math::Function stable = math::function({"x"}, [](math::Function x) { return x.expm1(); });
  const math::Function naive = math::function({"x"}, [](math::Function x) { return x.exp() - 1.0; });
  const double x = 1e-10;
  out.writeln("expm1(x):     ", stable(x).item<double>());
  out.writeln("exp(x) - 1:   ", naive(x).item<double>());
  out.writeln("x + x^2/2:    ", x + 0.5 * x * x);
}
```

Output

```text
expm1(x):      0.000000000100000000
exp(x) - 1:    0.000000000100000008
x + x^2/2:     0.000000000100000000
```

### Example 4

`expm1` and `log1p` are inverses: composing them returns the argument, including very small and negative values.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const math::Function there_and_back = math::function({"x"}, [](math::Function x) { return x.expm1().log1p(); });
  const auto x = pluss::make_series({-0.9, -1e-8, 0.0, 1e-8, 2.5});
  out.writeln("x:                ", x.items<double>());
  out.writeln("log1p(expm1(x)):  ", there_and_back({x}).items<double>());
}
```

Output

```text
x:                 [-0.9000000000, -0.0000000100, 0.0000000000, 0.0000000100, 2.5000000000]
log1p(expm1(x)):   [-0.9000000000, -0.0000000100, 0.0000000000, 0.0000000100, 2.5000000000]
```

## See Also

- [`exp`](exp.md): the exponential
- [`log1p`](log1p.md): the inverse

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="exp.html">
    <span class="label">Previous</span>
    <span class="title">exp</span>
  </a>
  <a class="next-link" href="imag.html">
    <span class="label">Next</span>
    <span class="title">imag</span>
  </a>
</div>
<!-- apiref:nav:end -->
