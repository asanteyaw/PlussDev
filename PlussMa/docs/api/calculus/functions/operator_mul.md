<!-- generated from operator_mul.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `math::operator*`

The infix product: a function times a function, a real number or a complex number, in either order.

## Signature

```cpp
math::Function math::operator*(const math::Function &lhs, const math::Function &rhs);
math::Function math::operator*(const math::Function &lhs, double rhs);
math::Function math::operator*(double lhs, const math::Function &rhs);
math::Function math::operator*(const math::Function &lhs, std::complex<double> rhs);
math::Function math::operator*(std::complex<double> lhs, const math::Function &rhs);
```

## What It Does

`f * g` is the elementwise product `x ↦ f(x) · g(x)`, and `f * 2.0` or `2.0 * f` scales every value. A complex factor makes the result complex-valued. The operator is identical to the named method [`mul`](Function/mul.md). It is the elementwise product: for matrices and tensors use [`matmul`](Function/matmul.md) or [`contract`](Function/contract.md).

Products are where weights and window functions enter a formula, and the product rule is applied exactly when the result is differentiated.

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

A new `Function` whose value is the elementwise product; complex if either operand is complex.

## Errors

- Functions from different traces: `expressions from different variable traces cannot be combined directly`.
- Both operands must be defined.

## Practical Examples

### Example 1

Function times function, and the number on either side.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function a = math::function({"x"}, [](math::Function x) { return x * x.exp(); });
  const math::Function b = math::function({"x"}, [](math::Function x) { return x * 3.0; });
  const math::Function c = math::function({"x"}, [](math::Function x) { return 3.0 * x; });
  const auto x = pluss::make_series({0.0, 1.0, 2.0});
  out.writeln("x e^x: ", a({x}).items<double>());
  out.writeln("x * 3: ", b({x}).items<double>());
  out.writeln("3 * x: ", c({x}).items<double>());
}
```

Output

```text
x e^x:  [0.0000, 2.7183, 14.7781]
x * 3:  [0.0000, 3.0000, 6.0000]
3 * x:  [0.0000, 3.0000, 6.0000]
```

### Example 2

Multiplying by a complex constant rotates and scales: `i · x` turns the real number `x` into the imaginary number `ix`.

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
  const std::complex<double> i{0.0, 1.0};
  const math::Function rotated = math::function({"x"}, [&](math::Function x) { return i * x; });
  const math::Function scaled = math::function({"x"}, [&](math::Function x) { return x * std::complex<double>{2.0, 1.0}; });
  const auto x = pluss::make_series({1.0, 2.0});
  out.writeln("i x:        real", rotated({x}).real().items<double>(), "imag", rotated({x}).imag().items<double>());
  out.writeln("x (2 + i):  real", scaled({x}).real().items<double>(), "imag", scaled({x}).imag().items<double>());
}
```

Output

```text
i x:        real [0.0000, 0.0000] imag [1.0000, 2.0000]
x (2 + i):  real [2.0000, 4.0000] imag [1.0000, 2.0000]
```

### Example 3

The product rule on a product of three factors: `(x · eˣ · sin x)′`, compared with the closed form.

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
  const math::Function f = math::function({"x"}, [](math::Function x) { return x * x.exp() * x.sin(); });
  const math::Function df = operators::differential("x")(f);
  const double x = 0.8;
  out.writeln("f'(0.8)                       =", df(x).item<double>());
  out.writeln("e^x (sin x + x sin x + x cos x)=", std::exp(x) * (std::sin(x) + x * std::sin(x) + x * std::cos(x)));
}
```

Output

```text
f'(0.8)                       = 4.1141
e^x (sin x + x sin x + x cos x)= 4.1141
```

## See Also

- [`mul`](Function/mul.md): the named method
- [`matmul`](Function/matmul.md): matrix product
- [`operator/`](operator_div.md): the quotient

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="operator_div.html">
    <span class="label">Previous</span>
    <span class="title">operator/</span>
  </a>
  <a class="next-link" href="operator_sub.html">
    <span class="label">Next</span>
    <span class="title">operator-</span>
  </a>
</div>
<!-- apiref:nav:end -->
