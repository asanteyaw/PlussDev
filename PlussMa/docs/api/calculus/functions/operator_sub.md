<!-- generated from operator_sub.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `math::operator-`

The infix difference: a function minus a function, a real number or a complex number, in either order.

## Signature

```cpp
math::Function math::operator-(const math::Function &lhs, const math::Function &rhs);
math::Function math::operator-(const math::Function &lhs, double rhs);
math::Function math::operator-(double lhs, const math::Function &rhs);
math::Function math::operator-(const math::Function &lhs, std::complex<double> rhs);
math::Function math::operator-(std::complex<double> lhs, const math::Function &rhs);
```

## What It Does

`f - g` is the difference function `x ↦ f(x) − g(x)`; `f - 2.0` subtracts a constant and `2.0 - f` subtracts the function from a constant. Unlike addition, the order matters: `2.0 - f` is `−(f − 2.0)`. The complex overloads work the same way and give a complex-valued function. It is identical to the named method [`sub`](Function/sub.md), which also describes the exact-zero simplification when both operands are the same node.

The unary form `-f` is a separate member; see [`negate`](Function/negate.md).

## Parameters

`const math::Function &lhs`
: The left operand. Two functions must share a variable trace (built in one `math::function(...)` callable, or derived from one another), or one must be a constant.

`const math::Function &rhs`
: The right operand.

`double rhs`
: A real number; the other operand is the function. `double lhs` is the same with the number on the left.

`std::complex<double> rhs`
: A complex number. The result is a complex-valued function. `std::complex<double> lhs` puts the number on the left.

## Returns

A new `Function` whose value is the difference; complex if either operand is complex.

## Errors

- Functions from different traces: `expressions from different variable traces cannot be combined directly`.
- Both operands must be defined.

## Practical Examples

### Example 1

The order matters: `f - 1` and `1 - f` are negatives of each other.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function a = math::function({"x"}, [](math::Function x) { return x * x - 1.0; });
  const math::Function b = math::function({"x"}, [](math::Function x) { return 1.0 - x * x; });
  const math::Function c = math::function({"x"}, [](math::Function x) { return x.exp() - x.sin(); });
  const auto x = pluss::make_series({0.0, 1.0, 2.0});
  out.writeln("x^2 - 1:      ", a({x}).items<double>());
  out.writeln("1 - x^2:      ", b({x}).items<double>());
  out.writeln("e^x - sin x:  ", c({x}).items<double>());
}
```

Output

```text
x^2 - 1:       [-1.0000, 0.0000, 3.0000]
1 - x^2:       [1.0000, 0.0000, -3.0000]
e^x - sin x:   [1.0000, 1.8768, 6.4798]
```

### Example 2

Complex constants on either side. `(1 + 2i) − x` and `x − (1 + 2i)` are negatives of each other.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const std::complex<double> w{1.0, 2.0};
  const math::Function right = math::function({"x"}, [&](math::Function x) { return x - w; });
  const math::Function left = math::function({"x"}, [&](math::Function x) { return w - x; });
  const auto x = pluss::make_series({3.0, 0.0});
  out.writeln("x - w: real", right({x}).real().items<double>(), "imag", right({x}).imag().items<double>());
  out.writeln("w - x: real", left({x}).real().items<double>(), "imag", left({x}).imag().items<double>());
}
```

Output

```text
x - w: real [2.0000, -1.0000] imag [-2.0000, -2.0000]
w - x: real [-2.0000, 1.0000] imag [2.0000, 2.0000]
```

### Example 3

A difference quotient. `shift` moves a function along a variable and keeps its trace, so `(f(x + h) − f(x)) / h` can be written directly, and it approaches the exact derivative as `h` shrinks.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.sin(); });
  const double exact = std::cos(1.0);
  for (const double h : {0.1, 0.01, 0.001}) {
    const math::Function quotient = (operators::shift("x", h)(f) - f) / h;
    out.writeln("h =", h, " quotient", quotient(1.0).item<double>(), " error", quotient(1.0).item<double>() - exact);
  }
}
```

Output

```text
h = 0.100000  quotient 0.497364  error -0.042939
h = 0.010000  quotient 0.536086  error -0.004216
h = 0.001000  quotient 0.539881  error -0.000421
```

## See Also

- [`sub`](Function/sub.md): the named method
- [`negate`](Function/negate.md): the unary minus
- [`operator+`](operator_add.md): the sum

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="operator_mul.html">
    <span class="label">Previous</span>
    <span class="title">operator*</span>
  </a>
  <a class="next-link" href="stack.html">
    <span class="label">Next</span>
    <span class="title">stack</span>
  </a>
</div>
<!-- apiref:nav:end -->
