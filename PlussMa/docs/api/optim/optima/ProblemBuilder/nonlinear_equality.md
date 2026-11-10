<!-- generated from nonlinear_equality.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1c7f5af6a193 -->

# `ProblemBuilder::nonlinear_equality`

[Back to `optim::ProblemBuilder`](../ProblemBuilder.md)

Adds the constraint `h(x) = 0`, differentiated automatically.

## Signature

```cpp
template <typename Fn> optim::ProblemBuilder &optim::ProblemBuilder::nonlinear_equality(Fn &&fn);
```

## What It Does

`fn` returns the residual `h(x)` of a constraint that must be zero, as a scalar or a vector. An equality is always active. Use it to confine the answer to a curve or surface, such as a unit circle `x₀² + x₁² − 1 = 0`. Several equalities are written as a vector of residuals; the number of equalities should not exceed the number of unknowns. [`nl_eq`](nl_eq.md) is the short name.

## Parameters

`Fn &&fn`
: Returns `h(x)`.

## Returns

The same builder, for chaining.

## Practical Examples

### Example 1

The point of the unit circle nearest `(3,3)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series target = pluss::series::make({3.0, 3.0});
  const optim::OptimaState result = optim::optima(optim::problem().objective([&](const Pack &p) { return (p.at("x") - target).square().sum(); })
      .nonlinear_equality([](const Pack &p) { return p.at("x").square().sum() - 1.0; }), pluss::series::make({1.0, 0.0}));
  out.writeln("x =", result.params.at("x").items<double>());
}
```

Output

```text
x = [0.7071, 0.7071]
```

### Example 2

Two scalars tied by a product. Minimise `a² + b²` with `ab = 4`: `a = b = 2`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const Pack start = optim::pack_scalars({{"a", pluss::make_series(1.0)}, {"b", pluss::make_series(3.0)}});
  const optim::OptimaState result = optim::optima(optim::problem().objective([](const Pack &p) { return p.at("a").square() + p.at("b").square(); })
      .nonlinear_equality([](const Pack &p) { return p.at("a") * p.at("b") - 4.0; }), start);
  out.writeln("a =", result.params.at("a").items<double>(), " b =", result.params.at("b").items<double>());
}
```

Output

```text
a = [2.0000]  b = [2.0000]
```

### Example 3

The multiplier of an equality measures how hard the constraint pushes. Minimise `x²` with `x = 2`: at the answer the objective has slope `2x = 4`, which the constraint balances, so the multiplier has magnitude 4 (its sign follows the convention `∇f + λ∇h = 0`).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const optim::OptimaState result = optim::optima(optim::problem().objective([](const Pack &p) { return p.at("x").square().sum(); })
      .nonlinear_equality([](const Pack &p) { return p.at("x").sum() - 2.0; }), pluss::series::make({0.0}));
  out.writeln("x =", result.params.at("x").items<double>());
  out.writeln("multiplier =", result.multipliers.nonlinear_equalities.items<double>());
}
```

Output

```text
x = [2.0000]
multiplier = [-4.0000]
```

## See Also

- [`nl_eq`](nl_eq.md): short name
- [`nonlinear_inequality`](nonlinear_inequality.md): `g(x) ≤ 0`
- [`ProblemBuilder`](Introduction.md): the builder
- [`optima`](../optima.md): solves the problem

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nonlinear_analytic.html">
    <span class="label">Previous</span>
    <span class="title">nonlinear_analytic</span>
  </a>
  <a class="next-link" href="nonlinear_inequality.html">
    <span class="label">Next</span>
    <span class="title">nonlinear_inequality</span>
  </a>
</div>
<!-- apiref:nav:end -->
