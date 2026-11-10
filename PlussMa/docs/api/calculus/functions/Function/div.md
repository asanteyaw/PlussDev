<!-- generated from div.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: be809c94ae4c -->

# `math::Function::div`

[Back to `math::Function`](../Function.md)

The quotient of two functions, or of a function and a number, as a new function.

## Signature

```cpp
math::Function math::Function::div(const math::Function &rhs) const;
math::Function math::Function::div(double rhs) const;
```

## What It Does

`f.div(g)` builds `x ↦ f(x) / g(x)` and `f.div(c)` divides by the number `c`; it is the same as the infix `f / g` (see [`operator/`](../operator_div.md)). The quotient rule is applied exactly when it is differentiated.

Division is the one arithmetic operation with a domain: a zero denominator is not a valid input. The function checks the denominators that actually arrive when it is evaluated and raises `division domain excludes zero denominators` instead of returning an infinity. Dividing a function by itself gives 1 only where the function is nonzero, so the library does not simplify `x / x` unless you declare `x` nonzero with [`assume`](assume.md).

## Parameters

`const math::Function &rhs`
: The denominator. Both operands must come from the same variable trace (built inside one `math::function(...)` callable, or derived from one another), or one of them must be a constant. Functions from unrelated `function(...)` calls cannot be combined directly; use [`compose`](compose.md) to substitute one into the other.

`double rhs`
: A number to divide by. Zero is refused at evaluation.

## Returns

A new `Function` whose value is the quotient.

## Errors

- A zero denominator at evaluation raises `division domain excludes zero denominators`.
- Functions from different traces are refused.

## Practical Examples

### Example 1

The quotient rule. `d/dx [sin x / x] = (x cos x − sin x) / x²`, evaluated and compared with the closed form.

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
  const math::Function sinc = math::function({"x"}, [](math::Function x) { return x.sin().div(x); });
  const math::Function d = operators::differential("x")(sinc);
  const double x = 2.0;
  out.writeln("sinc'(2)             =", d(x).item<double>());
  out.writeln("(x cos x - sin x)/x^2 =", (x * std::cos(x) - std::sin(x)) / (x * x));
}
```

Output

```text
sinc'(2)             = -0.4354
(x cos x - sin x)/x^2 = -0.4354
```

### Example 2

A zero denominator is refused at evaluation, naming the operation, rather than producing `inf` or `NaN`.

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
  const math::Function reciprocal = math::function({"x"}, [](math::Function x) { return math::function(1.0).div(x); });
  out.writeln("1/x on {2, 4}:", reciprocal({pluss::make_series({2.0, 4.0})}).items<double>());
  try {
    (void)reciprocal({pluss::make_series({2.0, 0.0})});
  } catch (const pluss::Error &e) {
    const std::string text(e.what());
    out.writeln("1/x on {2, 0}:", text.substr(0, text.find('\n')));
  }
}
```

Output

```text
1/x on {2, 4}: [0.5000, 0.2500]
1/x on {2, 0}: math::Function: division domain excludes zero denominators
```

### Example 3

Dividing by a number, and declaring a denominator nonzero so that `x / x` becomes the constant 1 (the promise is the caller's).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function half = math::function({"x"}, [](math::Function x) { return x.div(2.0); });
  const math::Function ratio = math::function({"x"}, [](math::Function x) { return x.div(x); }).assume("x", math::Assumption::NonZero);
  out.writeln("x / 2 at 5:        ", half(5.0).item<double>());
  out.writeln("x / x | NonZero at -3:", ratio(-3.0).item<double>());
}
```

Output

```text
x / 2 at 5:         2.5000
x / x | NonZero at -3: 1.0000
```

## See Also

- [`operator/`](../operator_div.md): the infix spelling
- [`assume`](assume.md): declare a denominator nonzero

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="det.html">
    <span class="label">Previous</span>
    <span class="title">det</span>
  </a>
  <a class="next-link" href="exp.html">
    <span class="label">Next</span>
    <span class="title">exp</span>
  </a>
</div>
<!-- apiref:nav:end -->
