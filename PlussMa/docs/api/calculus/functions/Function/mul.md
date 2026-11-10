<!-- generated from mul.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 064f9886295a -->

# `math::Function::mul`

[Back to `math::Function`](../Function.md)

The product of two functions, or of a function and a number, as a new function.

## Signature

```cpp
math::Function math::Function::mul(const math::Function &rhs) const;
math::Function math::Function::mul(double rhs) const;
```

## What It Does

`f.mul(g)` builds `x ↦ f(x) · g(x)` and `f.mul(c)` scales by the number `c`. It is the same operation as the infix `f * g` (see [`operator*`](../operator_mul.md)). The product is elementwise: it multiplies matching components. Matrix and tensor products are the separate [`matmul`](matmul.md) and [`contract`](contract.md).

Multiplication is how weights, densities and window functions enter a formula, and it is where the product rule lives: differentiating `f.mul(g)` produces `f' g + f g'` exactly. Two simplifications apply at construction: multiplying by an exact zero gives zeros of the common shape, even at points where `0 · ∞` would be `NaN`, and a sum of two products that share a factor is factored.

## Parameters

`const math::Function &rhs`
: The factor. Both operands must come from the same variable trace (built inside one `math::function(...)` callable, or derived from one another), or one of them must be a constant. Functions from unrelated `function(...)` calls cannot be combined directly; use [`compose`](compose.md) to substitute one into the other.

`double rhs`
: A number that scales the value at every point.

## Returns

A new `Function` whose value is the elementwise product.

## Errors

- Functions from different traces: `expressions from different variable traces cannot be combined directly`.
- Operand shapes must broadcast.

## Practical Examples

### Example 1

The product rule is applied to the recorded graph, so the derivative of a product is exact. Here `(x sin x)′ = sin x + x cos x`.

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
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.mul(x.sin()); });
  const math::Function df = operators::differential("x")(f);
  const double x = 1.3;
  out.writeln("f'(1.3)            =", df(x).item<double>());
  out.writeln("sin x + x cos x    =", std::sin(x) + x * std::cos(x));
}
```

Output

```text
f'(1.3)            = 1.3113
sin x + x cos x    = 1.3113
```

### Example 2

Scaling by a number, and a zero factor. Multiplying by an exact zero gives an exact zero even where the other factor is infinite.

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
  const math::Function scaled = math::function({"x"}, [](math::Function x) { return x.mul(2.5); });
  const math::Function zeroed = math::function({"x"}, [](math::Function x) { return x.mul(0.0); });
  const auto x = pluss::make_series({1.0, 4.0, std::numeric_limits<double>::infinity()});
  out.writeln("2.5 x: ", scaled({x}).items<double>());
  out.writeln("0 * x: ", zeroed({x}).items<double>());
}
```

Output

```text
2.5 x:  [2.5000, 10.0000, inf]
0 * x:  [0.0000, 0.0000, 0.0000]
```

Floating point gives `0 · ∞ = NaN`; the simplified function returns 0.

### Example 3

The product is elementwise: a vector of two components times a scalar function scales both, and a vector times a vector multiplies matching components.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function w = math::function({"t"}, [](math::Function t) {
    const math::Function v = math::stack({t, t + 1.0});
    return v.mul(math::stack({t * 0.0 + 2.0, t * 0.0 + 3.0}));
  });
  const math::Function scaled = math::function({"t"}, [](math::Function t) { return math::stack({t, t + 1.0}).mul(t); });
  out.writeln("v .* [2, 3] at 4:", w(4.0).items<double>());
  out.writeln("v * t at 4:      ", scaled(4.0).items<double>());
}
```

Output

```text
v .* [2, 3] at 4: [8.0000, 15.0000]
v * t at 4:       [16.0000, 20.0000]
```

`[4, 5] ⊙ [2, 3] = [8, 15]` and `[4, 5] · 4 = [16, 20]`.

## See Also

- [`operator*`](../operator_mul.md): the infix spelling
- [`matmul`](matmul.md): matrix product
- [`div`](div.md): the quotient

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="matmul.html">
    <span class="label">Previous</span>
    <span class="title">matmul</span>
  </a>
  <a class="next-link" href="negate.html">
    <span class="label">Next</span>
    <span class="title">negate</span>
  </a>
</div>
<!-- apiref:nav:end -->
