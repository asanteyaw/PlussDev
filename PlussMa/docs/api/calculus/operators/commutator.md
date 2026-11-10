<!-- generated from commutator.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::commutator`

The commutator `[A, B] = AB − BA` of two operators.

## Signature

```cpp
operators::Operator operators::commutator(const operators::Operator &left, const operators::Operator &right);
```

## What It Does

`commutator(A, B)` is the operator `f ↦ A(B f) − B(A f)`. It measures how much the order of two operators matters: it is the zero operator exactly when they commute. Its best-known instance is `[d/dx, x] = I`: differentiating after multiplying by `x` differs from multiplying after differentiating by exactly the function itself, which is the Heisenberg relation between momentum and position.

Commutators appear in the Lie bracket of vector fields, in the error of operator splitting methods, and in showing that two symmetries are compatible. The commutator of two linear operators is linear; it is not savable (it is built from a pair), and its adjoint is not built automatically.

## Parameters

`const operators::Operator &left`
: The operator `A`.

`const operators::Operator &right`
: The operator `B`.

## Returns

An `Operator` equal to `AB − BA`.

## Errors

- Both operators must be defined.

## Practical Examples

### Example 1

The canonical relation `[d/dx, x] = I`. Applied to any function it returns that function, here `x³` and `sin x`.

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
  const operators::Operator D = operators::differential("x");
  const operators::Operator X = operators::multiply(math::function({"x"}, [](math::Function x) { return x; }));
  const operators::Operator bracket = operators::commutator(D, X);
  const math::Function a = math::function({"x"}, [](math::Function x) { return x * x * x; });
  const math::Function b = math::function({"x"}, [](math::Function x) { return x.sin(); });
  out.writeln("[D, X] x^3 at 2   =", bracket(a)(2.0).item<double>(), "  (x^3 = 8)");
  out.writeln("[D, X] sin at 2   =", bracket(b)(2.0).item<double>(), "  (sin 2 =", std::sin(2.0), ")");
}
```

Output

```text
[D, X] x^3 at 2   = 8.0000   (x^3 = 8)
[D, X] sin at 2   = 0.9093   (sin 2 = 0.9093 )
```

### Example 2

Operators that commute have the zero commutator: a derivative and a scaling, two derivatives, and a derivative and a translation.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x * x * x + x.sin(); });
  const operators::Operator D = operators::differential("x");
  out.writeln("[D, scale(3)] f at 1:   ", operators::commutator(D, operators::scale(3.0))(f)(1.0).item<double>());
  out.writeln("[D, D^2] f at 1:        ", operators::commutator(D, operators::differential("x", 2))(f)(1.0).item<double>());
  out.writeln("[D, shift(0.5)] f at 1: ", operators::commutator(D, operators::shift("x", 0.5))(f)(1.0).item<double>());
}
```

Output

```text
[D, scale(3)] f at 1:    0.0000000000
[D, D^2] f at 1:         0.0000000000
[D, shift(0.5)] f at 1:  0.0000000000
```

### Example 3

A non-zero case: `[d/dx, x²] = 2x`, the derivative of the multiplier.

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
  const operators::Operator D = operators::differential("x");
  const operators::Operator X2 = operators::multiply(math::function({"x"}, [](math::Function x) { return x * x; }));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.exp(); });
  out.writeln("[D, x^2] f at 1 =", operators::commutator(D, X2)(f)(1.0).item<double>(), "  (2x e^x = 2e =", 2.0 * std::exp(1.0), ")");
  out.writeln("linear:", operators::commutator(D, X2).capabilities().linear);
}
```

Output

```text
[D, x^2] f at 1 = 5.4366   (2x e^x = 2e = 5.4366 )
linear: true
```

## See Also

- [`compose`](compose.md): forms `AB`
- [`linear_combination`](linear_combination.md): forms the difference

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bind.html">
    <span class="label">Previous</span>
    <span class="title">bind</span>
  </a>
  <a class="next-link" href="compose.html">
    <span class="label">Next</span>
    <span class="title">compose</span>
  </a>
</div>
<!-- apiref:nav:end -->
