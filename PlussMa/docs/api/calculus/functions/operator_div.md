<!-- generated from operator_div.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `math::operator/`

The infix quotient: a function divided by a function, a real number or a complex number, or a number divided by a function.

## Signature

```cpp
math::Function math::operator/(const math::Function &lhs, const math::Function &rhs);
math::Function math::operator/(const math::Function &lhs, double rhs);
math::Function math::operator/(double lhs, const math::Function &rhs);
math::Function math::operator/(const math::Function &lhs, std::complex<double> rhs);
math::Function math::operator/(std::complex<double> lhs, const math::Function &rhs);
```

## What It Does

`f / g` is the quotient function `x ↦ f(x) / g(x)`; `f / 2.0` divides by a constant and `2.0 / f` is the reciprocal scaled by a constant. As with subtraction the order matters. The operator is identical to the named method [`div`](Function/div.md). A zero denominator is not an input: it is refused when the function is evaluated, with a message that names the operation, rather than returning an infinity.

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

A new `Function` whose value is the quotient; complex if either operand is complex.

## Errors

- A zero denominator at evaluation raises `division domain excludes zero denominators`.
- Functions from different traces are refused.

## Practical Examples

### Example 1

The order matters: `f / 2` and `2 / f` are different functions.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function a = math::function({"x"}, [](math::Function x) { return x / 2.0; });
  const math::Function b = math::function({"x"}, [](math::Function x) { return 2.0 / x; });
  const math::Function c = math::function({"x"}, [](math::Function x) { return x.sin() / x; });
  const auto x = pluss::make_series({1.0, 2.0, 4.0});
  out.writeln("x / 2:    ", a({x}).items<double>());
  out.writeln("2 / x:    ", b({x}).items<double>());
  out.writeln("sin x / x:", c({x}).items<double>());
}
```

Output

```text
x / 2:     [0.5000, 1.0000, 2.0000]
2 / x:     [2.0000, 1.0000, 0.5000]
sin x / x: [0.8415, 0.4546, -0.1892]
```

### Example 2

Complex constants: dividing by `1 + i` and dividing `1 + i` by the function.

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
  const std::complex<double> w{1.0, 1.0};
  const math::Function right = math::function({"x"}, [&](math::Function x) { return x / w; });
  const math::Function left = math::function({"x"}, [&](math::Function x) { return w / x; });
  const auto x = pluss::make_series({2.0, 4.0});
  out.writeln("x / (1+i): real", right({x}).real().items<double>(), "imag", right({x}).imag().items<double>());
  out.writeln("(1+i) / x: real", left({x}).real().items<double>(), "imag", left({x}).imag().items<double>());
}
```

Output

```text
x / (1+i): real [1.0000, 2.0000] imag [-1.0000, -2.0000]
(1+i) / x: real [0.5000, 0.2500] imag [0.5000, 0.2500]
```

### Example 3

A rational function, its derivative by the quotient rule, and the refusal of a zero denominator.

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
  const math::Function r = math::function({"x"}, [](math::Function x) { return (x * x + 1.0) / (x - 1.0); });
  const math::Function dr = operators::differential("x")(r);
  out.writeln("r(3)  =", r(3.0).item<double>(), "  (10 / 2)");
  out.writeln("r'(3) =", dr(3.0).item<double>(), "  ((2x(x-1) - (x^2+1)) / (x-1)^2 = 2/4)");
  try {
    (void)r(1.0);
  } catch (const pluss::Error &e) {
    const std::string text(e.what());
    out.writeln("r(1): ", text.substr(0, text.find('\n')));
  }
}
```

Output

```text
r(3)  = 5.0000   (10 / 2)
r'(3) = 0.5000   ((2x(x-1) - (x^2+1)) / (x-1)^2 = 2/4)
r(1):  math::Function: division domain excludes zero denominators
```

## See Also

- [`div`](Function/div.md): the named method
- [`operator*`](operator_mul.md): the product

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="operator_add.html">
    <span class="label">Previous</span>
    <span class="title">operator+</span>
  </a>
  <a class="next-link" href="operator_mul.html">
    <span class="label">Next</span>
    <span class="title">operator*</span>
  </a>
</div>
<!-- apiref:nav:end -->
