<!-- generated from differential.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::fractional::differential`

The fractional derivative of order `α > 0`, in the Caputo, Riemann–Liouville or Grünwald–Letnikov sense.

## Signature

```cpp
operators::Operator operators::fractional::differential(std::string_view variable, double alpha, operators::fractional::Kind kind = operators::fractional::Kind::Caputo, double lower = 0.0);
```

## What It Does

`differential("x", α)` is the fractional derivative of order `α`, with `m = ⌈α⌉`. There are three definitions, chosen by `kind`:

| `Kind` | Definition | On a constant |
|---|---|---|
| `Caputo` (default) | `I^{m−α} f⁽ᵐ⁾`: differentiate `m` times, then take a fractional integral | `0` |
| `RiemannLiouville` | `dᵐ/dxᵐ I^{m−α} f`: fractional integral, then differentiate `m` times | `x^{−α}/Γ(1−α)` |
| `GrunwaldLetnikov` | the limit of fractional differences; equal to Riemann–Liouville for the functions it is defined on | `x^{−α}/Γ(1−α)` |

The kinds agree on functions that vanish at the lower terminal with all their derivatives, and differ on constants. The Caputo derivative of a constant is zero, which is why it is the one used with initial conditions in differential equations; the Riemann–Liouville derivative of a constant is singular at the terminal. Polynomials with a zero terminal get exact power-law results, `D^α x^p = Γ(p+1)/Γ(p−α+1) x^{p−α}`; other functions are unresolved integrals evaluated by quadrature. The order must be positive and the terminal finite.

## Parameters

`std::string_view variable`
: The variable of the function and the evaluation point.

`double alpha`
: The order, positive.

`operators::fractional::Kind kind`
: `Caputo` (default), `RiemannLiouville` or `GrunwaldLetnikov`.

`double lower`
: The lower terminal `a`; finite, default 0.

## Returns

An `Operator` whose application returns `D^α f` as a `Function` of the variable.

## Errors

- `needs a variable, alpha > 0 and a finite lower terminal`.

## Practical Examples

### Example 1

The three kinds on the constant 1 at `x = 4`: Riemann–Liouville and Grünwald–Letnikov give `x^{−1/2}/Γ(1/2) = 0.28209`; Caputo gives 0.

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

  out.writeln("Riemann-Liouville:", fractional::differential("x", 0.5, fractional::Kind::RiemannLiouville)(one)(4.0).item<double>());
  out.writeln("Caputo:           ", fractional::differential("x", 0.5, fractional::Kind::Caputo)(one)(4.0).item<double>());
  out.writeln("Grunwald-Letnikov:", fractional::differential("x", 0.5, fractional::Kind::GrunwaldLetnikov)(one)(4.0).item<double>());
  out.writeln("exact 1/sqrt(pi x):", 1.0 / std::sqrt(pi * 4.0));
}
```

Output

```text
Riemann-Liouville: 0.28209
Caputo:            0.00000
Grunwald-Letnikov: 0.28209
exact 1/sqrt(pi x): 0.28209
```

### Example 2

A power: `D^{1/2} x = Γ(2)/Γ(3/2) x^{1/2} = 2√(x/π)`, which is `2.25676` at `x = 4` (Caputo, the default kind). A derivative of order 1 is the ordinary derivative: `D¹ x² = 2x`.

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
  const math::Function identity_fn = math::function({"x"}, [](math::Function x) { return x; });

  out.writeln("D^0.5 of x at 4:", fractional::differential("x", 0.5)(identity_fn)(4.0).item<double>(), "  2 sqrt(x/pi) =", 2.0 * std::sqrt(4.0 / pi));
  const math::Function square = math::function({"x"}, [](math::Function x) { return x * x; });
  out.writeln("D^1 of x^2 at 3:", fractional::differential("x", 1.0)(square)(3.0).item<double>(), "  (2x = 6)");
}
```

Output

```text
D^0.5 of x at 4: 2.25676   2 sqrt(x/pi) = 2.25676
D^1 of x^2 at 3: 6.00000   (2x = 6)
```

### Example 3

The derivative undoes the integral. Applying the Riemann–Liouville half-derivative to the half-integral of `x²` returns `x²`: at `x = 3` it is 9.

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
  const math::Function square = math::function({"x"}, [](math::Function x) { return x * x; });
  const operators::Operator round_trip = operators::compose(fractional::differential("x", 0.5, fractional::Kind::RiemannLiouville), fractional::integral("x", 0.5));
  out.writeln("D^(1/2) I^(1/2) x^2 at 3:", round_trip(square)(3.0).item<double>(), "  (9)");
}
```

Output

```text
D^(1/2) I^(1/2) x^2 at 3: 9.00000   (9)
```

### Example 4

A lower terminal other than 0, and the checks. With terminal 1 the power is measured from 1: `D^{1/2}(x − 1)` at `x = 5` is `2√(4/π)`. The order must be positive.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace fractional = pluss::operators::fractional;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const double pi = std::acos(-1.0);
  const auto first_line = [](const pluss::Error &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  const math::Function f = math::function({"x"}, [](math::Function x) { return x - 1.0; });
  out.writeln("D^0.5 (x - 1), terminal 1, at 5:", fractional::differential("x", 0.5, fractional::Kind::Caputo, 1.0)(f)(5.0).item<double>(), "  2 sqrt(4/pi) =", 2.0 * std::sqrt(4.0 / pi));
  try { (void)fractional::differential("x", -1.0); } catch (const pluss::Error &e) { out.writeln("alpha < 0:", first_line(e)); }
}
```

Output

```text
D^0.5 (x - 1), terminal 1, at 5: 2.25676   2 sqrt(4/pi) = 2.25676
alpha < 0: operators::fractional: needs a variable, alpha > 0 and a finite lower terminal
```

## See Also

- [`integral`](integral.md): the fractional integral
- [`differential`](../differential/differential.md): integer-order derivatives

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="integral.html">
    <span class="label">Next</span>
    <span class="title">integral</span>
  </a>
</div>
<!-- apiref:nav:end -->
