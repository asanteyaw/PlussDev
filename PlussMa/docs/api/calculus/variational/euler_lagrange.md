<!-- generated from euler_lagrange.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::variational::euler_lagrange`

The Euler–Lagrange expression of a Lagrangian: the function whose zeros are the stationary functions.

## Signature

```cpp
operators::Operator operators::variational::euler_lagrange(std::string_view independent, pluss::StackArray<std::string_view> jets, operators::DiffOptions options = {});
```

## What It Does

A function `u(x)` makes the functional `F[u] = ∫ L dx` stationary exactly when `δF/δu = 0`, the *Euler–Lagrange equation*. `euler_lagrange` returns that expression for a given Lagrangian: it is the same operator as [`functional_derivative`](functional_derivative.md) (the two have the same key and behaviour), named for the use you put it to. Applied to `L` it gives `E = Σₖ (−D_x)ᵏ ∂L/∂uₖ` as a function of the independent variable and the jets; a candidate `u(x)` solves the problem when `E` vanishes along it.

To test a candidate, evaluate the expression on the candidate and its derivatives, supplied as `Series`: `x`, `u(x)`, `u′(x)`, `u″(x)`, and so on. The residual is zero (to rounding) for a solution.

A Lagrangian density `L(x, u, u′, u″, …)` is written as a function of the independent variable and of *jet variables*: `jets = {u, u1, u2, …}`, each its own variable standing for `u` and its successive derivatives. List at least twice as many orders as `L` uses, since the `k`-th order contributes `(−d/dx)ᵏ` applied to a derivative that itself involves orders up to `k` higher.

## Parameters

`std::string_view independent`
: The independent variable `x`.

`pluss::StackArray<std::string_view> jets`
: The jet variables `{u, u′, u″, …}`, at least two.

`operators::DiffOptions options`
: Options for the derivatives.

## Returns

An `Operator` whose application returns the Euler–Lagrange expression as a scalar `Function` of `x` and the jets.

## Errors

- `needs an independent variable and at least u and u'`.
- `the Lagrangian must be scalar-valued`.

## Practical Examples

### Example 1

A candidate solution of the harmonic oscillator. For `L = ½u′² − ½u²`, `E = −u − u″`. The function `u = sin x` has `u″ = −sin x`, so the residual is zero; `u = sin 2x` has `u″ = −4 sin 2x` and is not a solution.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const math::Function L = math::function({"x", "u", "u1", "u2", "u3"}, [](math::Function, math::Function u, math::Function u1, math::Function, math::Function) { return 0.5 * u1 * u1 - 0.5 * u * u; });
  const math::Function E_L = operators::variational::euler_lagrange("x", {"u", "u1", "u2", "u3"})(L);
  const auto x = pluss::make_series({0.3, 1.0, 2.5});
  const auto zero = x * 0.0;
  out.writeln("u = sin x:   residual", E_L({x, x.sin(), x.cos(), -x.sin(), -x.cos()}).items<double>());
  out.writeln("u = sin 2x:  residual", E_L({x, (2.0 * x).sin(), 2.0 * (2.0 * x).cos(), -4.0 * (2.0 * x).sin(), -8.0 * (2.0 * x).cos()}).items<double>());
}
```

Output

```text
u = sin x:   residual [0.00000000, 0.00000000, 0.00000000]
u = sin 2x:  residual [1.69392742, 2.72789228, -2.87677282]
```

### Example 2

The beam equation: a Lagrangian with a second derivative. `L = ½u″² − q(x) u` gives `E = u⁗ − q`. The jets must go up to fourth order. For the load `q = x`, the function `u = x⁵/120` has `u⁗ = x` and solves it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const math::Function L = math::function({"x", "u", "u1", "u2", "u3", "u4"}, [](math::Function x, math::Function u, math::Function, math::Function u2, math::Function, math::Function) { return 0.5 * u2 * u2 - x * u; });
  const math::Function E_L = operators::variational::euler_lagrange("x", {"u", "u1", "u2", "u3", "u4"})(L);
  const auto x = pluss::make_series({0.5, 1.0, 2.0});
  const auto u = x.pow(5.0) / 120.0, u1 = x.pow(4.0) / 24.0, u2 = x.pow(3.0) / 6.0, u3 = x * x / 2.0, u4 = x;
  out.writeln("residual of u = x^5/120:", E_L({x, u, u1, u2, u3, u4}).items<double>());
}
```

Output

```text
residual of u = x^5/120: [0.0000000000, 0.0000000000, 0.0000000000]
```

### Example 3

The shortest path between two points. The arc length of a graph, `L = √(1 + u′²)`, gives `E = −u″/(1 + u′²)^{3/2}`: zero for a straight line (`u″ = 0`), non-zero for a curve. At `u′ = 1, u″ = 2` it is `−2/2^{3/2} = −0.7071`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const math::Function L = math::function({"x", "u", "u1", "u2", "u3"}, [](math::Function, math::Function, math::Function u1, math::Function, math::Function) { return (1.0 + u1 * u1).sqrt(); });
  const math::Function E_L = operators::variational::euler_lagrange("x", {"u", "u1", "u2", "u3"})(L);
  const auto at = [&](double u1, double u2) { return E_L({pluss::make_series(0.0), pluss::make_series(0.0), pluss::make_series(u1), pluss::make_series(u2), pluss::make_series(0.0)}).item<double>(); };
  out.writeln("straight line (u'' = 0):", std::abs(at(1.0, 0.0)));
  out.writeln("curve (u' = 1, u'' = 2):", at(1.0, 2.0), "  (-2 / 2^1.5 = -0.70711)");
}
```

Output

```text
straight line (u'' = 0): 0.00000
curve (u' = 1, u'' = 2): -0.70711   (-2 / 2^1.5 = -0.70711)
```

### Example 4

It is the same operator as `functional_derivative`: same key, same result.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const math::Function L = math::function({"x", "u", "u1", "u2"}, [](math::Function, math::Function u, math::Function u1, math::Function) { return 0.5 * u1 * u1 - 0.5 * u * u; });
  const auto a = operators::variational::euler_lagrange("x", {"u", "u1", "u2"});
  const auto b = operators::variational::functional_derivative("x", {"u", "u1", "u2"});
  const auto point = {pluss::make_series(0.0), pluss::make_series(2.0), pluss::make_series(0.0), pluss::make_series(-2.0)};
  out.writeln("keys:", a.key(), "/", b.key());
  out.writeln("values:", a(L)(point).item<double>(), b(L)(point).item<double>());
}
```

Output

```text
keys: variational_functional_derivative / variational_functional_derivative
values: 0.00000 0.00000
```

## See Also

- [`functional_derivative`](functional_derivative.md): the same operator under its other name

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="functional_derivative.html">
    <span class="label">Next</span>
    <span class="title">functional_derivative</span>
  </a>
</div>
<!-- apiref:nav:end -->
