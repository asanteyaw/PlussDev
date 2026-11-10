<!-- generated from operator_add.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `math::operator+`

The infix sum: a function plus a function, a real number or a complex number, in either order.

## Signature

```cpp
math::Function math::operator+(const math::Function &lhs, const math::Function &rhs);
math::Function math::operator+(const math::Function &lhs, double rhs);
math::Function math::operator+(double lhs, const math::Function &rhs);
math::Function math::operator+(const math::Function &lhs, std::complex<double> rhs);
math::Function math::operator+(std::complex<double> lhs, const math::Function &rhs);
```

## What It Does

`f + g` is the sum function `x ↦ f(x) + g(x)`, and `f + 2.0` and `2.0 + f` add a constant to every value. The function operand may be on either side, since addition commutes. Nothing is evaluated until the result is called; the operator only records the sum. It is identical to the named method [`add`](Function/add.md).

The complex overloads exist so that complex constants can be written directly: `f + std::complex<double>{0.0, 1.0}` is a complex-valued function whose imaginary part is shifted by one. Adding a complex number to a real function makes the result complex.

Operators between two functions require them to belong to one variable trace; see [`add`](Function/add.md) for what that means and how to combine functions that do not.

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

A new `Function` whose value is the sum; complex if either operand is complex.

## Errors

- Functions from different traces: `expressions from different variable traces cannot be combined directly`.
- Both operands must be defined.

## Practical Examples

### Example 1

The three real forms: function plus function, function plus number, number plus function. The two with a number agree.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x"}, [](math::Function x) {
    return x.sin() + x * x;  // function + function
  });
  const math::Function right = math::function({"x"}, [](math::Function x) { return x * x + 3.0; });
  const math::Function left = math::function({"x"}, [](math::Function x) { return 3.0 + x * x; });
  const auto x = pluss::make_series({0.0, 1.0, 2.0});
  out.writeln("sin x + x^2:  ", f({x}).items<double>());
  out.writeln("x^2 + 3:      ", right({x}).items<double>());
  out.writeln("3 + x^2:      ", left({x}).items<double>());
}
```

Output

```text
sin x + x^2:   [0.0000, 1.8415, 4.9093]
x^2 + 3:       [3.0000, 4.0000, 7.0000]
3 + x^2:       [3.0000, 4.0000, 7.0000]
```

### Example 2

Complex constants. Adding `i` to a real function gives a complex-valued function; the real part is unchanged and the imaginary part is shifted.

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
  const math::Function f = math::function({"x"}, [&](math::Function x) { return x * x + i; });
  const math::Function g = math::function({"x"}, [&](math::Function x) { return std::complex<double>{1.0, -2.0} + x; });
  const auto x = pluss::make_series({1.0, 2.0});
  out.writeln("x^2 + i:   real", f({x}).real().items<double>(), "imag", f({x}).imag().items<double>());
  out.writeln("(1-2i) + x: real", g({x}).real().items<double>(), "imag", g({x}).imag().items<double>());
}
```

Output

```text
x^2 + i:   real [1.0000, 4.0000] imag [1.0000, 1.0000]
(1-2i) + x: real [2.0000, 3.0000] imag [-2.0000, -2.0000]
```

### Example 3

Building a polynomial and differentiating it: sums of terms differentiate term by term.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function p = math::function({"x"}, [](math::Function x) { return 1.0 + 2.0 * x + 3.0 * x * x + 4.0 * x * x * x; });
  const math::Function dp = operators::differential("x")(p);
  out.writeln("p(2)  =", p(2.0).item<double>(), "  (1 + 4 + 12 + 32 = 49)");
  out.writeln("p'(2) =", dp(2.0).item<double>(), "  (2 + 12 + 48 = 62)");
}
```

Output

```text
p(2)  = 49.0000   (1 + 4 + 12 + 32 = 49)
p'(2) = 62.0000   (2 + 12 + 48 = 62)
```

## See Also

- [`add`](Function/add.md): the named method
- [`operator-`](operator_sub.md): the difference
- [`operator*`](operator_mul.md): the product

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="function_factory.html">
    <span class="label">Previous</span>
    <span class="title">function</span>
  </a>
  <a class="next-link" href="operator_div.html">
    <span class="label">Next</span>
    <span class="title">operator/</span>
  </a>
</div>
<!-- apiref:nav:end -->
