<!-- generated from negate.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f44d8b3bf401 -->

# `math::Function::operator-`

[Back to `math::Function`](../Function.md)

The unary minus, `operator-`: the negative of a function, `−f`.

## Signature

```cpp
math::Function math::Function::operator-() const;
```

## What It Does

The unary minus builds `x ↦ −f(x)`. It negates every component of a vector or matrix value, and for a complex function negates both parts. Applying it twice is simplified away, so `-(-f)` is `f` without any extra node.

Use it where the sign matters structurally rather than numerically: an energy and its negative, the loss `−log L` that an optimizer minimizes, a force as the negative gradient of a potential. Differentiation commutes with it, `(−f)′ = −f′`, so a function and its negation never need separate handling.

## Returns

A new `Function` whose value is the negative.

## Errors

- The function must be defined.

## Practical Examples

### Example 1

Negation, and the equality of `−f` with multiplying by −1.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.exp() - x; });
  const math::Function g = -f;
  const auto x = pluss::make_series({0.0, 1.0, 2.0});
  out.writeln("f:    ", f({x}).items<double>());
  out.writeln("-f:   ", g({x}).items<double>());
  out.writeln("-1 * f:", f.mul(-1.0)({x}).items<double>());
}
```

Output

```text
f:     [1.0000, 1.7183, 5.3891]
-f:    [-1.0000, -1.7183, -5.3891]
-1 * f: [-1.0000, -1.7183, -5.3891]
```

### Example 2

A force as the negative gradient of a potential, `V = x²/2 + y²`: `F = −∇V`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function potential = math::function({"x", "y"}, [](math::Function x, math::Function y) { return 0.5 * x * x + y * y; });
  const math::Function force = -operators::gradient()(potential);
  out.writeln("F at (2, 3):", force({pluss::make_series(2.0), pluss::make_series(3.0)}).items<double>());
}
```

Output

```text
F at (2, 3): [-2.0000, -6.0000]
```

`−∇V = (−x, −2y) = (−2, −6)`.

### Example 3

A double negation leaves no trace, and negation reaches every component of a vector function.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function v = math::function({"t"}, [](math::Function t) { return -math::stack({t, t * t, -t}); });
  const math::Function plain = math::function({"t"}, [](math::Function t) { return -(-t); });
  out.writeln("-[t, t^2, -t] at 2:", v(2.0).items<double>());
  out.writeln("-(-t) at 2:        ", plain(2.0).item<double>());
}
```

Output

```text
-[t, t^2, -t] at 2: [-2.0000, -4.0000, 2.0000]
-(-t) at 2:         2.0000
```

## See Also

- [`sub`](sub.md): subtraction
- [`abs`](abs.md): absolute value

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mul.html">
    <span class="label">Previous</span>
    <span class="title">mul</span>
  </a>
  <a class="next-link" href="pow.html">
    <span class="label">Next</span>
    <span class="title">pow</span>
  </a>
</div>
<!-- apiref:nav:end -->
