<!-- generated from taylor_operator.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::expansion::taylor`

The truncated Taylor expansion of a function: a polynomial function that approximates it near a point.

## Signature

```cpp
operators::Operator operators::expansion::taylor(std::string_view variable, double center, std::int64_t order, operators::DiffOptions options = {}, operators::expansion::Engine engine = operators::expansion::Engine::Auto);
operators::Operator operators::expansion::taylor(pluss::StackArray<std::string_view> variables, pluss::RangeView<double> center, std::int64_t order, operators::DiffOptions options = {});
operators::expansion::Taylor operators::expansion::taylor();
```

## What It Does

Near a point `a` a smooth function is well approximated by its Taylor polynomial `Tₙ(x) = Σ_{k≤n} f⁽ᵏ⁾(a)/k! (x − a)ᵏ`. There are three forms of `taylor`.

**One variable**, `taylor("x", a, n)`: the operator that turns `f` into `Tₙ`, a polynomial `Function` of `x`. Other variables of `f` stay free, so the expansion of `f(x, a)` in `x` is a function of both. **Several variables**, `taylor({"x", "y"}, {a, b}, n)`: the multivariate polynomial `Σ_{|α|≤n} ∂ᵅf(a)/α! Π (xᵢ − aᵢ)^αᵢ` over all multi-indices of total degree up to `n`. **No arguments**, `taylor()`: returns the [`Taylor`](Taylor.md) object whose methods give the coefficients and the remainder.

The result is an ordinary analytic function: it evaluates, differentiates, integrates, composes and saves. The order must lie in `[0, 64]`; order 0 is the constant `f(a)`, a function of no `x`. How the derivatives are found is the `Engine`:

| `Engine` | How | Best for |
|---|---|---|
| `Structural` | builds the derivative graph exactly | low orders; handles vector-valued functions; the graph grows with the order |
| `Jet` | carries truncated power series through the formula; cost grows with the square of the order | high orders; scalar elementwise functions only |
| `Auto` (default) | `Structural` up to order 8, `Jet` above it when the function allows | everything |

All three give the same polynomial (to rounding); the multi-variable form uses the structural method.

## Parameters

`std::string_view variable`
: The variable to expand in.

`double center`
: The expansion point `a`.

`std::int64_t order`
: The truncation order `n`, in `[0, 64]`.

`operators::DiffOptions options`
: Options for the derivatives.

`operators::expansion::Engine engine`
: `Auto` (default), `Structural` or `Jet`.

`pluss::StackArray<std::string_view> variables`
: The variables of the multivariate form.

`pluss::RangeView<double> center`
: The expansion point, one entry per variable (multivariate form).

## Returns

An `Operator` whose application returns the polynomial as a `Function`. The no-argument form returns a `Taylor` object.

## Errors

- `order must lie in [0, 64]`.
- `one centre per variable is required` for the multivariate form.
- `the power-series engine supports scalar elementwise Functions only` for `Engine::Jet` with a vector-valued function.
- An unknown variable raises `unknown variable '<name>'`.

## Practical Examples

### Example 1

An order-3 polynomial of `exp` about 0: `1 + x + x²/2 + x³/6`. At `x = 0.5` it is `1.64583`, against the true `1.64872`. The result is an ordinary exact function (`Structural`) of the same variable.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace expansion = pluss::operators::expansion;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.exp(); });

  const math::Function t3 = operators::expansion::taylor("x", 0.0, 3)(f);
  out.writeln("T3(0.5)   =", t3(0.5).item<double>(), "  1 + .5 + .125 + .5^3/6 =", 1.0 + 0.5 + 0.125 + 0.125 / 6.0);
  out.writeln("exp(0.5)  =", f(0.5).item<double>());
  out.writeln("provenance Structural:", t3.provenance() == math::Provenance::Structural, "  variables:", t3.variables()[0]);
}
```

Output

```text
T3(0.5)   = 1.64583   1 + .5 + .125 + .5^3/6 = 1.64583
exp(0.5)  = 1.64872
provenance Structural: true   variables: x
```

### Example 2

The error shrinks with the order. At `x = 1` the polynomials of orders 1, 2, 4, 8 and 12 approach `e`: the error falls like `1/(n+1)!`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace expansion = pluss::operators::expansion;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.exp(); });

  for (const std::int64_t order : {1, 2, 4, 8, 12}) {
    const math::Function T = operators::expansion::taylor("x", 0.0, order)(f);
    out.writeln("order", order, ": error at x = 1:", std::exp(1.0) - T(1.0).item<double>());
  }
}
```

Output

```text
order 1 : error at x = 1: 0.7182818285
order 2 : error at x = 1: 0.2182818285
order 4 : error at x = 1: 0.0099484951
order 8 : error at x = 1: 0.0000030586
order 12 : error at x = 1: 0.0000000002
```

### Example 3

A centre other than 0. About `a = 1`, `log` has the series `(x−1) − (x−1)²/2 + (x−1)³/3 − …`; the order-4 polynomial at `x = 1.2` is `0.18227`, against `log 1.2 = 0.18232`. And order 0 is the constant `f(a)`: a function of no variable, called with `()`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace expansion = pluss::operators::expansion;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const math::Function g = math::function({"x"}, [](math::Function x) { return x.log(); });
  const math::Function T4 = operators::expansion::taylor("x", 1.0, 4)(g);
  out.writeln("T4(1.2) =", T4(1.2).item<double>(), "  log(1.2) =", std::log(1.2));
  const math::Function T0 = operators::expansion::taylor("x", 1.0, 0)(g);
  out.writeln("order 0: arity", T0.arity(), " value", T0().item<double>(), "  (log 1 = 0)");
}
```

Output

```text
T4(1.2) = 0.18227   log(1.2) = 0.18232
order 0: arity 0  value 0.00000   (log 1 = 0)
```

### Example 4

Several variables. `exp(xy)` about `(0, 0)` to total degree 4 is `1 + xy + (xy)²/2` (the next term has degree 6): at `(0.5, 0.4)` that is `1.22`, against `exp(0.2) = 1.22140`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace expansion = pluss::operators::expansion;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const math::Function h = math::function({"x", "y"}, [](math::Function x, math::Function y) { return (x * y).exp(); });
  const std::vector<double> centre = {0.0, 0.0};
  const math::Function T = operators::expansion::taylor({"x", "y"}, pluss::RangeView<double>(centre), 4)(h);
  out.writeln("T(0.5, 0.4) =", T({pluss::make_series(0.5), pluss::make_series(0.4)}).item<double>(), "  exp(0.2) =", std::exp(0.2));
}
```

Output

```text
T(0.5, 0.4) = 1.22000   exp(0.2) = 1.22140
```

### Example 5

Other variables stay free. The expansion of `exp(a x)` in `x` is a function of `x` and `a`: `Σ aᵏ xᵏ/k!`. At `x = 0.5` for three values of `a`, order 6.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace expansion = pluss::operators::expansion;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function f = math::function({"x", "a"}, [](math::Function x, math::Function a) { return (a * x).exp(); });
  const math::Function T = operators::expansion::taylor("x", 0.0, 6)(f);
  out.writeln("variables:", T.variables()[0], T.variables()[1]);
  const auto a = pluss::make_series({0.5, 1.0, 2.0});
  out.writeln("T(0.5, a): ", T({pluss::make_series({0.5, 0.5, 0.5}), a}).items<double>());
  out.writeln("exp(a x):  ", (a * 0.5).exp().items<double>());
}
```

Output

```text
variables: x a
T(0.5, a):  [1.284025, 1.648720, 2.718056]
exp(a x):   [1.284025, 1.648721, 2.718282]
```

### Example 6

The three engines agree. At order 12 (where `Auto` would pick `Jet`) `Auto`, `Structural` and `Jet` all reproduce `e` at `x = 1` to the printed digits. `Jet` is for scalar elementwise functions and refuses a vector-valued one; `Structural` expands it componentwise.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace expansion = pluss::operators::expansion;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto first_line = [](const pluss::Error &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.exp(); });

  for (const auto &[label, engine] : {std::pair<const char *, expansion::Engine>{"Auto      ", expansion::Engine::Auto}, {"Structural", expansion::Engine::Structural}, {"Jet       ", expansion::Engine::Jet}}) {
    out.writeln(label, operators::expansion::taylor("x", 0.0, 12, {}, engine)(f)(1.0).item<double>());
  }
  const math::Function pair = math::function({"x"}, [](math::Function x) { return math::stack({x.exp(), x.sin()}); });
  out.writeln("vector-valued, Structural, shape", operators::expansion::taylor("x", 0.0, 3)(pair).shape(), operators::expansion::taylor("x", 0.0, 3)(pair)(0.5).items<double>());
  try { (void)operators::expansion::taylor("x", 0.0, 12, {}, expansion::Engine::Jet)(pair); } catch (const pluss::Error &e) { out.writeln("vector-valued, Jet:", first_line(e)); }
}
```

Output

```text
Auto       2.71828183
Structural 2.71828183
Jet        2.71828183
vector-valued, Structural, shape [2] [1.64583333, 0.47916667]
vector-valued, Jet: operators::expansion::taylor: the power-series engine supports scalar elementwise Functions only
```

### Example 7

The checks: the order range, one centre per variable, and the variable must exist.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace expansion = pluss::operators::expansion;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto first_line = [](const pluss::Error &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.exp(); });

  const math::Function h = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x * y; });
  const std::vector<double> one_centre = {0.0};
  try { (void)operators::expansion::taylor("x", 0.0, -1); } catch (const pluss::Error &e) { out.writeln("order -1:        ", first_line(e)); }
  try { (void)operators::expansion::taylor({"x", "y"}, pluss::RangeView<double>(one_centre), 2)(h); } catch (const pluss::Error &e) { out.writeln("centres:         ", first_line(e)); }
  try { (void)operators::expansion::taylor("z", 0.0, 2)(f); } catch (const pluss::Error &e) { out.writeln("unknown variable: ", first_line(e)); }
}
```

Output

```text
order -1:         operators::expansion::taylor: order must lie in [0, 64]
centres:          operators::expansion::taylor: one centre per variable is required
unknown variable:  math::Function: unknown variable 'z'
```

## See Also

- [`Taylor`](Taylor.md): coefficients and remainder
- [`power`](power_operator.md): builds a polynomial from coefficients
- [`differential`](../differential/differential.md): the derivatives the coefficients are built from

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="power_operator.html">
    <span class="label">Previous</span>
    <span class="title">power</span>
  </a>
</div>
<!-- apiref:nav:end -->
