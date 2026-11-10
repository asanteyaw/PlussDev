<!-- generated from sub.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8f461cb1c88f -->

# `math::Function::sub`

[Back to `math::Function`](../Function.md)

The difference of two functions, or of a function and a number, as a new function.

## Signature

```cpp
math::Function math::Function::sub(const math::Function &rhs) const;
math::Function math::Function::sub(double rhs) const;
```

## What It Does

`f.sub(g)` builds `x ↦ f(x) − g(x)`; `f.sub(c)` subtracts the number `c` at every point. It is the same operation as the infix `f - g` (see [`operator-`](../operator_sub.md)), and like [`add`](add.md) it returns a new function without evaluating anything.

The method is the right tool when a difference is the quantity you want to study: an error between a model and a target, a residual, or the gap between two expressions. Because a function and its derivative share a trace, `f.sub(df)` is also well defined.

One useful property: when both operands are the *same recorded node*, the difference is simplified to an exact zero at construction. That matters at points where floating point gives `NaN`, such as `∞ − ∞`; see the second example. Two separately written expressions that look alike are different nodes and are subtracted numerically.

## Parameters

`const math::Function &rhs`
: The function to subtract. Both operands must come from the same variable trace (built inside one `math::function(...)` callable, or derived from one another), or one of them must be a constant. Functions from unrelated `function(...)` calls cannot be combined directly; use [`compose`](compose.md) to substitute one into the other.

`double rhs`
: A number subtracted at every point and component.

## Returns

A new `Function` whose value is the difference.

## Errors

- Functions from different traces: `expressions from different variable traces cannot be combined directly`.
- Both operands must be defined.

## Practical Examples

### Example 1

A model against a target, built in one callable, and the residual's derivative. The method and the operator agree.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function residual = math::function({"x"}, [](math::Function x) { return x.exp().sub(1.0 + x + 0.5 * x * x); });
  const math::Function infix = math::function({"x"}, [](math::Function x) { return x.exp() - (1.0 + x + 0.5 * x * x); });
  const auto x = pluss::make_series({0.0, 0.1, 0.5, 1.0});
  out.writeln("e^x - (1 + x + x^2/2):", residual({x}).items<double>());
  out.writeln("operator -:          ", infix({x}).items<double>());
  out.writeln("derivative:          ", operators::differential("x")(residual)({x}).items<double>());
}
```

Output

```text
e^x - (1 + x + x^2/2): [0.0000, 0.0002, 0.0237, 0.2183]
operator -:           [0.0000, 0.0002, 0.0237, 0.2183]
derivative:           [0.0000, 0.0052, 0.1487, 0.7183]
```

The residual of the second-order Taylor polynomial grows like `x³/6`; its derivative `eˣ − 1 − x` grows like `x²/2`.

### Example 2

Subtracting a number shifts the function down. Subtracting a function from itself is an exact zero even at infinity, where the arithmetic `∞ − ∞` would be `NaN`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <limits>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.sub(2.0); });
  const math::Function same = math::function({"x"}, [](math::Function x) { const math::Function s = x * x; return s.sub(s); });
  const math::Function twice = math::function({"x"}, [](math::Function x) { return (x * x).sub(x * x); });
  const auto x = pluss::make_series({3.0, std::numeric_limits<double>::infinity()});
  out.writeln("x - 2:                     ", f({x}).items<double>());
  out.writeln("s - s (one node):           ", same({x}).items<double>());
  out.writeln("x*x - x*x (two nodes):      ", twice({x}).items<double>());
}
```

Output

```text
x - 2:                      [1.0000, inf]
s - s (one node):            [0.0000, 0.0000]
x*x - x*x (two nodes):       [0.0000, nan]
```

Naming a sub-expression once and reusing it is what makes the two operands one node.

### Example 3

A vector-valued function minus a scalar subtracts from every component; functions from unrelated `function(...)` calls cannot be subtracted.

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
  const math::Function v = math::function({"t"}, [](math::Function t) { return math::stack({t, t * t}); });
  out.writeln("v(3):     ", v(3.0).items<double>());
  out.writeln("v(3) - 1: ", v.sub(1.0)(3.0).items<double>());
  try {
    (void)v.sub(math::function({"t"}, [](math::Function t) { return t.cos(); }));
  } catch (const pluss::Error &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
v(3):      [3.0000, 9.0000]
v(3) - 1:  [2.0000, 8.0000]
math::Function: expressions from different variable traces cannot be combined directly
```

The scalar is subtracted from each component. Build the operands in one callable, or use `compose`, when they start separate.

## See Also

- [`operator-`](../operator_sub.md): the infix spelling and both operand orders
- [`add`](add.md): the sum

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sqrt.html">
    <span class="label">Previous</span>
    <span class="title">sqrt</span>
  </a>
  <a class="next-link" href="tan.html">
    <span class="label">Next</span>
    <span class="title">tan</span>
  </a>
</div>
<!-- apiref:nav:end -->
