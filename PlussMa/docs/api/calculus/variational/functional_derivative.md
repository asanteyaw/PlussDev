<!-- generated from functional_derivative.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::variational::functional_derivative`

The functional derivative `δF/δu` of the functional `F[u] = ∫ L(x, u, u′, …) dx`.

## Signature

```cpp
operators::Operator operators::variational::functional_derivative(std::string_view independent, pluss::StackArray<std::string_view> jets, operators::DiffOptions options = {});
```

## What It Does

For a functional `F[u] = ∫ L(x, u, u′, u″, …) dx` the *functional derivative* `δF/δu` is the function that gives the first-order change of `F` for a small change `δu`: `δF = ∫ (δF/δu) δu dx`. By integrating by parts,

`δF/δu = Σₖ (−D_x)ᵏ ∂L/∂uₖ`, where `D_x g = ∂g/∂x + Σₖ uₖ₊₁ ∂g/∂uₖ` is the total derivative.

`functional_derivative("x", {"u", "u1", "u2", "u3", "u4"})` is the operator that takes the Lagrangian `L` and returns `δF/δu` as a function of `x` and the jet variables.

A Lagrangian density `L(x, u, u′, u″, …)` is written as a function of the independent variable and of *jet variables*: `jets = {u, u1, u2, …}`, each its own variable standing for `u` and its successive derivatives. List at least twice as many orders as `L` uses, since the `k`-th order contributes `(−d/dx)ᵏ` applied to a derivative that itself involves orders up to `k` higher.

The independent variable must be non-empty and `jets` must name at least `u` and `u′`. Use it for the gradient of an energy functional, the force in a field theory, and the variational derivative in gradient flows.

## Parameters

`std::string_view independent`
: The independent variable `x` of the Lagrangian.

`pluss::StackArray<std::string_view> jets`
: The jet variables `{u, u′, u″, …}`, at least two, each a variable of the Lagrangian. List at least twice as many orders as the Lagrangian uses.

`operators::DiffOptions options`
: Options for the derivatives.

## Returns

An `Operator` whose application returns `δF/δu` as a scalar `Function` of `x` and the jets.

## Errors

- `needs an independent variable and at least u and u'`.
- `the Lagrangian must be scalar-valued`.

## Practical Examples

### Example 1

The harmonic oscillator, `L = ½u′² − ½u²`: `δF/δu = −u − u″`. Evaluated on `u = 2, u″ = −2` (so `u″ = −u`) it vanishes: that is the equation of motion.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function L = math::function({"x", "u", "u1", "u2", "u3"}, [](math::Function, math::Function u, math::Function u1, math::Function, math::Function) { return 0.5 * u1 * u1 - 0.5 * u * u; });
  const math::Function delta = operators::variational::functional_derivative("x", {"u", "u1", "u2", "u3"})(L);
  out.writeln("variables:", delta.arity(), "  (x, u, u1, u2, u3)");
  const auto at = [&](double u, double u2) { return delta({pluss::make_series(0.0), pluss::make_series(u), pluss::make_series(0.0), pluss::make_series(u2), pluss::make_series(0.0)}).item<double>(); };
  out.writeln("u = 2, u'' = -2:", at(2.0, -2.0), "  (-u - u'' = 0)");
  out.writeln("u = 3, u'' = -2:", at(3.0, -2.0), "  (-u - u'' = -1)");
}
```

Output

```text
variables: 5   (x, u, u1, u2, u3)
u = 2, u'' = -2: 0.0000   (-u - u'' = 0)
u = 3, u'' = -2: -1.0000   (-u - u'' = -1)
```

### Example 2

A gradient-energy functional (Ginzburg–Landau type): `L = ½κ u′² + ½a u² + ¼b u⁴` gives `δF/δu = −κ u″ + a u + b u³`. At `u = 1, u″ = 0.5` with `κ = 2, a = 1, b = 3` that is `−1 + 1 + 3 = 3`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const double kappa = 2.0, a = 1.0, b = 3.0;
  const math::Function L = math::function({"x", "u", "u1", "u2", "u3"}, [&](math::Function, math::Function u, math::Function u1, math::Function, math::Function) {
    return 0.5 * kappa * u1 * u1 + 0.5 * a * u * u + 0.25 * b * u * u * u * u;
  });
  const math::Function delta = operators::variational::functional_derivative("x", {"u", "u1", "u2", "u3"})(L);
  out.writeln("delta F / delta u =", delta({pluss::make_series(0.0), pluss::make_series(1.0), pluss::make_series(0.3), pluss::make_series(0.5), pluss::make_series(0.0)}).item<double>(), "  (-2 * 0.5 + 1 + 3 = 3)");
}
```

Output

```text
delta F / delta u = 3.0000   (-2 * 0.5 + 1 + 3 = 3)
```

### Example 3

Explicit dependence on `x` and a non-quadratic Lagrangian. For `L = x u′ + u²/2`: `δF/δu = u − D_x(x) = u − 1`. The total derivative of `x` is 1.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function L = math::function({"x", "u", "u1", "u2", "u3"}, [](math::Function x, math::Function u, math::Function u1, math::Function, math::Function) { return x * u1 + 0.5 * u * u; });
  const math::Function delta = operators::variational::functional_derivative("x", {"u", "u1", "u2", "u3"})(L);
  out.writeln("at u = 4:", delta({pluss::make_series(2.0), pluss::make_series(4.0), pluss::make_series(0.0), pluss::make_series(0.0), pluss::make_series(0.0)}).item<double>(), "  (u - 1 = 3)");
}
```

Output

```text
at u = 4: 3.0000   (u - 1 = 3)
```

### Example 4

The checks: the independent variable must be named, at least two jets (`u` and `u′`) are needed, and the Lagrangian must be scalar-valued.

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

  const math::Function L = math::function({"x", "u"}, [](math::Function, math::Function u) { return u * u; });
  try { (void)operators::variational::functional_derivative("", {"u", "u1"}); } catch (const pluss::Error &e) { out.writeln("no independent variable:", first_line(e)); }
  try { (void)operators::variational::functional_derivative("x", {"u"}); } catch (const pluss::Error &e) { out.writeln("only one jet:          ", first_line(e)); }
  const math::Function vector_valued = math::function({"x", "u", "u1"}, [](math::Function, math::Function u, math::Function u1) { return math::stack({u, u1}); });
  try { (void)operators::variational::functional_derivative("x", {"u", "u1"})(vector_valued); } catch (const pluss::Error &e) { out.writeln("vector Lagrangian:     ", first_line(e)); }
}
```

Output

```text
no independent variable: operators::variational: needs an independent variable and at least u and u'
only one jet:           operators::variational: needs an independent variable and at least u and u'
vector Lagrangian:      operators::variational: the Lagrangian must be scalar-valued
```

## See Also

- [`euler_lagrange`](euler_lagrange.md): the equation `δF/δu = 0`
- [`differential`](../differential/differential.md): the derivatives it is built from

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="euler_lagrange.html">
    <span class="label">Previous</span>
    <span class="title">euler_lagrange</span>
  </a>
</div>
<!-- apiref:nav:end -->
