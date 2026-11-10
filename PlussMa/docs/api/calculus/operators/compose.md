<!-- generated from compose.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::compose`

Chains two operators into one: first `inner`, then `outer`.

## Signature

```cpp
operators::Operator operators::compose(const operators::Operator &outer, const operators::Operator &inner);
```

## What It Does

`compose(outer, inner)` is the operator `f ↦ outer(inner(f))`: the *inner* operator acts first, as in mathematical notation `(A∘B) f = A(B f)`. The result is a single `Operator`, so it can be applied, saved, combined again, and given to anything that takes an operator. Chains nest and flatten: `compose(A, compose(B, C))` and `compose(compose(A, B), C)` are the same chain, and the identity is dropped.

Order matters in general. `d/dx` after multiplying by `x` is not `x` after differentiating; that difference is exactly the [`commutator`](commutator.md). The capabilities of the chain follow its parts: it is linear only if every part is, and savable only if every part is.

## Parameters

`const operators::Operator &outer`
: The operator applied second.

`const operators::Operator &inner`
: The operator applied first.

## Returns

A single `Operator` equal to `outer ∘ inner`.

## Errors

- Both operators must be defined.

## Practical Examples

### Example 1

The inner operator acts first. Differentiating after multiplying by `x` gives `(x f)′`; multiplying after differentiating gives `x f′`. For `f = x²` at `x = 2` those are `12` and `8`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x * x; });
  const operators::Operator D = operators::differential("x");
  const operators::Operator X = operators::multiply(math::function({"x"}, [](math::Function x) { return x; }));
  out.writeln("compose(D, X) f at 2 = (x f)' =", operators::compose(D, X)(f)(2.0).item<double>());
  out.writeln("compose(X, D) f at 2 = x f'   =", operators::compose(X, D)(f)(2.0).item<double>());
}
```

Output

```text
compose(D, X) f at 2 = (x f)' = 12.0000
compose(X, D) f at 2 = x f'   = 8.0000
```

### Example 2

Chains are flat and the identity disappears. Three different groupings of the same three operators agree, and their key is `compose`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x * x * x; });
  const operators::Operator A = operators::scale(2.0), B = operators::differential("x"), C = operators::shift("x", 1.0);
  const double left = operators::compose(operators::compose(A, B), C)(f)(1.0).item<double>();
  const double right = operators::compose(A, operators::compose(B, C))(f)(1.0).item<double>();
  const double padded = operators::compose(operators::identity(), operators::compose(A, operators::compose(B, C)))(f)(1.0).item<double>();
  out.writeln("(A.B).C =", left, "  A.(B.C) =", right, "  I.A.B.C =", padded, "  key:", operators::compose(A, B).key());
}
```

Output

```text
(A.B).C = 24.0000   A.(B.C) = 24.0000   I.A.B.C = 24.0000   key: compose
```

`2 · d/dx (x+1)³` at 1 is `2 · 3 · 4 = 24`.

### Example 3

A polynomial in an operator: `D² + 2D + I` built as a combination of compositions, applied to `e^{−x}`, annihilates it: `(D + I)² e^{−x} = 0`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function decay = math::function({"x"}, [](math::Function x) { return (-x).exp(); });
  const operators::Operator D = operators::differential("x");
  const operators::Operator I = operators::identity();
  const operators::Operator D_plus_I = operators::linear_combination({{1.0, D}, {1.0, I}});
  const operators::Operator squared = operators::compose(D_plus_I, D_plus_I);
  out.writeln("(D + I)^2 e^{-x} at 0.7 =", squared(decay)(0.7).item<double>());
  out.writeln("(D + I) e^{-x} at 0.7   =", D_plus_I(decay)(0.7).item<double>());
  out.writeln("linear:", squared.capabilities().linear, "  serializable:", squared.capabilities().serializable);
}
```

Output

```text
(D + I)^2 e^{-x} at 0.7 = 0.0000
(D + I) e^{-x} at 0.7   = 0.0000
linear: true   serializable: false
```

### Example 4

Both operators must be defined.

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
  const auto first_line = [](const pluss::Error &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  try { (void)operators::compose(operators::differential("x"), operators::Operator{}); } catch (const pluss::Error &e) { out.writeln(first_line(e)); }
}
```

Output

```text
operators::Operator: operation requires a defined operator
```

## See Also

- [`linear_combination`](linear_combination.md): sums operators
- [`commutator`](commutator.md): measures how order matters
- [`identity`](identity.md): dropped from chains

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="commutator.html">
    <span class="label">Previous</span>
    <span class="title">commutator</span>
  </a>
  <a class="next-link" href="define.html">
    <span class="label">Next</span>
    <span class="title">define</span>
  </a>
</div>
<!-- apiref:nav:end -->
