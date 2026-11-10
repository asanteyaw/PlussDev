<!-- generated from adjoint.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::adjoint`

The formal adjoint of an operator: the operator `A*` with `⟨A f, g⟩ = ⟨f, A* g⟩`.

## Signature

```cpp
operators::Operator operators::adjoint(const operators::Operator &op);
```

## What It Does

`adjoint(A)` builds the operator `A*` that moves `A` from one factor of an inner product to the other. In `L²` for functions that vanish at the ends of the domain (so integration by parts drops its boundary terms), the adjoint of `d/dx` is `−d/dx`, of a real multiplication is itself, of `shift(a)` is `shift(−a)`; the adjoint of a composition reverses the order, `(AB)* = B*A*`, and the adjoint of a linear combination combines the adjoints with the same coefficients.

Adjoints are how a differential operator is turned into the one in the weak form of a problem, how the Fokker–Planck operator is derived from a generator, and how a Green's identity is written. Only operators that declare `adjoint_known` can be adjoint-ed (see [`capabilities`](Operator/capabilities.md)); asking for the adjoint of any other is an error that names the operator.

## Parameters

`const operators::Operator &op`
: The operator to take the adjoint of. It must declare `adjoint_known`, directly or through its parts.

## Returns

An `Operator` that is the formal adjoint.

## Errors

- An operator without a known adjoint raises `operator '<key>' has no known adjoint`.
- The operator must be defined.

## Practical Examples

### Example 1

The adjoint of the derivative is minus the derivative, and the adjoint of the second derivative is itself. Applied to `x³` at 2: `−3x² = −12` and `6x = 12`.

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

  out.writeln("adjoint(D) x^3 at 2   =", operators::adjoint(operators::differential("x"))(f)(2.0).item<double>());
  out.writeln("adjoint(D^2) x^3 at 2 =", operators::adjoint(operators::differential("x", 2))(f)(2.0).item<double>());
  out.writeln("adjoint(shift 1) x^3 at 2 =", operators::adjoint(operators::shift("x", 1.0))(f)(2.0).item<double>(), "  (f(2 - 1) = 1)");
}
```

Output

```text
adjoint(D) x^3 at 2   = -12.0000
adjoint(D^2) x^3 at 2 = 12.0000
adjoint(shift 1) x^3 at 2 = 1.0000   (f(2 - 1) = 1)
```

### Example 2

The defining identity, checked numerically. For two functions that vanish at the ends of a wide interval, `∫ (A f) g dx = ∫ f (A* g) dx`. Here `A = d/dx · x` (multiply by `x`, then differentiate), so `A* = x · (−d/dx)`: the order reverses.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const math::Function f = math::function({"x"}, [](math::Function x) { return (-x * x).exp(); });
  const math::Function g = math::function({"x"}, [](math::Function x) { return (-(x - 0.5) * (x - 0.5)).exp() * x; });
  const operators::Operator X = operators::multiply(math::function({"x"}, [](math::Function x) { return x; }));
  const operators::Operator A = operators::compose(operators::differential("x"), X);
  const operators::Operator A_star = operators::adjoint(A);
  const operators::Operator integrate = operators::integral("x", -8.0, 8.0);
  out.writeln("<A f, g>    =", integrate(operators::multiply(g)(A(f)))().item<double>());
  out.writeln("<f, A* g>   =", integrate(operators::multiply(f)(A_star(g)))().item<double>());
}
```

Output

```text
<A f, g>    = -0.17281966
<f, A* g>   = -0.17281966
```

### Example 3

An operator that does not know its adjoint is refused, naming it. The magnitude of a derivative is not linear and declares none.

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

  try {
    (void)operators::adjoint(operators::magnitude_differential("x"));
  } catch (const pluss::Error &e) {
    out.writeln(first_line(e));
  }
  out.writeln("adjoint_known for differential:", operators::differential("x").capabilities().adjoint_known);
  out.writeln("adjoint_known for gradient:    ", operators::gradient().capabilities().adjoint_known);
}
```

Output

```text
operators::adjoint: operator 'magnitude_differential' has no known adjoint
adjoint_known for differential: true
adjoint_known for gradient:     false
```

## See Also

- [`capabilities`](Operator/capabilities.md): reports `adjoint_known`
- [`compose`](compose.md): adjoints reverse composition
- [`differential`](../differential/differential.md): the derivative, whose adjoint is its negative

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Options.html">
    <span class="label">Previous</span>
    <span class="title">Options</span>
  </a>
  <a class="next-link" href="bind.html">
    <span class="label">Next</span>
    <span class="title">bind</span>
  </a>
</div>
<!-- apiref:nav:end -->
