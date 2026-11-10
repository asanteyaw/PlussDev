<!-- generated from integral.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::fractional::integral`

The Riemann–Liouville fractional integral of order `α > 0`: the `α`-fold integral, for non-integer `α`.

## Signature

```cpp
operators::Operator operators::fractional::integral(std::string_view variable, double alpha, double lower = 0.0);
```

## What It Does

`integral("x", α)` is the operator

`(I^α f)(x) = 1/Γ(α) ∫_a^x (x − t)^{α−1} f(t) dt`,

with lower terminal `a` (default 0). For a whole number `α = n` it is `n` repeated integrals, by the Cauchy formula for repeated integration; for other `α` it is the natural interpolation. It is a smoothing operator that remembers the whole history of `f` from `a` to `x`: a model for processes with memory, such as viscoelastic materials and anomalous diffusion. The semigroup law `I^α I^β = I^{α+β}` holds.

The result is a function of `x` (the evaluation point, `x > a`). The kernel is singular at `t = x`, so the implementation substitutes to remove the endpoint singularity; polynomials with `a = 0` get an exact power-law closed form `I^α x^p = Γ(p+1)/Γ(p+α+1) x^{p+α}`, and other integrands stay unresolved integrals evaluated by quadrature. `alpha` must be positive and the terminal finite.

## Parameters

`std::string_view variable`
: The variable of the function and the evaluation point.

`double alpha`
: The order, positive.

`double lower`
: The lower terminal `a`; finite, default 0. The result is defined for `x > a`.

## Returns

An `Operator` whose application returns `I^α f` as a `Function` of the variable.

## Errors

- `needs a variable, alpha > 0 and a finite lower terminal`.

## Practical Examples

### Example 1

The half-integral of the constant 1: `I^{1/2} 1 = x^{1/2}/Γ(3/2) = 2√x/√π`, which is `2.25676` at `x = 4`. The order-1 integral is the ordinary one: `I¹ x = x²/2`, which is 2 at `x = 2`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace fractional = pluss::operators::fractional;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const double pi = std::acos(-1.0);
  const math::Function one = math::function({"x"}, [](math::Function x) { return x * 0.0 + 1.0; });
  const math::Function identity_fn = math::function({"x"}, [](math::Function x) { return x; });

  out.writeln("I^0.5 of 1 at 4: ", fractional::integral("x", 0.5)(one)(4.0).item<double>(), "  2 sqrt(4)/sqrt(pi) =", 4.0 / std::sqrt(pi));
  out.writeln("I^1 of x at 2:   ", fractional::integral("x", 1.0)(identity_fn)(2.0).item<double>(), "  x^2/2 = 2");
}
```

Output

```text
I^0.5 of 1 at 4:  2.25676   2 sqrt(4)/sqrt(pi) = 2.25676
I^1 of x at 2:    2.00000   x^2/2 = 2
```

### Example 2

A different lower terminal. With `a = 1`, `I^{1/2} 1 = 2√(x − 1)/√π`: the same function shifted. At `x = 5` it is again `2.25676`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace fractional = pluss::operators::fractional;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const double pi = std::acos(-1.0);
  const math::Function one = math::function({"x"}, [](math::Function x) { return x * 0.0 + 1.0; });

  out.writeln("terminal 0, at 4:", fractional::integral("x", 0.5, 0.0)(one)(4.0).item<double>());
  out.writeln("terminal 1, at 5:", fractional::integral("x", 0.5, 1.0)(one)(5.0).item<double>(), "  (same, shifted by 1)");
}
```

Output

```text
terminal 0, at 4: 2.25676
terminal 1, at 5: 2.25676   (same, shifted by 1)
```

### Example 3

The semigroup law. Applying the half-integral twice is the first-order integral: `I^{1/2} I^{1/2} x = x²/2`, which is 2 at `x = 2`; a third-order integral of a constant is `x³/6`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace fractional = pluss::operators::fractional;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const math::Function one = math::function({"x"}, [](math::Function x) { return x * 0.0 + 1.0; });
  const math::Function identity_fn = math::function({"x"}, [](math::Function x) { return x; });

  const operators::Operator half = fractional::integral("x", 0.5);
  out.writeln("I^(1/2) I^(1/2) x at 2:", operators::compose(half, half)(identity_fn)(2.0).item<double>(), "  (x^2/2 = 2)");
  out.writeln("I^3 of 1 at 2:         ", fractional::integral("x", 3.0)(one)(2.0).item<double>(), "  (x^3/6 = 1.3333)");
}
```

Output

```text
I^(1/2) I^(1/2) x at 2: 2.00000   (x^2/2 = 2)
I^3 of 1 at 2:          1.33333   (x^3/6 = 1.3333)
```

### Example 4

The checks: the order must be positive and the lower terminal finite.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <limits>
#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace fractional = pluss::operators::fractional;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto first_line = [](const pluss::Error &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  try { (void)fractional::integral("x", 0.0); } catch (const pluss::Error &e) { out.writeln("alpha = 0:       ", first_line(e)); }
  try { (void)fractional::integral("x", 0.5, -std::numeric_limits<double>::infinity()); } catch (const pluss::Error &e) { out.writeln("infinite terminal:", first_line(e)); }
}
```

Output

```text
alpha = 0:        operators::fractional: needs a variable, alpha > 0 and a finite lower terminal
infinite terminal: operators::fractional: needs a variable, alpha > 0 and a finite lower terminal
```

## See Also

- [`differential`](differential.md): fractional derivatives
- [`integral`](../integral/integral.md): the ordinary integral

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="differential.html">
    <span class="label">Previous</span>
    <span class="title">differential</span>
  </a>
</div>
<!-- apiref:nav:end -->
