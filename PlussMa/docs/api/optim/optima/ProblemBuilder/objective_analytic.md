<!-- generated from objective_analytic.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3cf58b0699b3 -->

# `ProblemBuilder::objective_analytic`

[Back to `optim::ProblemBuilder`](../ProblemBuilder.md)

Sets the objective to a function that returns the value together with its gradient.

## Signature

```cpp
optim::ProblemBuilder &optim::ProblemBuilder::objective_analytic(std::function<std::pair<pluss::Series, optim::ParameterPack<pluss::Series>>(const optim::ParameterPack<pluss::Series> &)> fn);
```

## What It Does

Use it when the gradient has a closed form that is cheaper or more accurate than recording the computation. `fn` returns `(value, gradient)`, where the gradient is a pack with exactly the layout of the parameters (`optim::pack_series(g)` for a single block). Nothing is differentiated automatically for the objective. If the gradient is wrong, the solver converges to a wrong point or stalls; [`audit_optima_derivatives`](../audit_optima_derivatives.md) compares an analytic gradient with the automatic one.

## Parameters

`fn`
: Returns `(value, gradient)` for a parameter pack.

## Returns

The same builder, for chaining.

## Practical Examples

### Example 1

A quadratic with its gradient. `Σ (xᵢ − cᵢ)²` has gradient `2(x − c)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series c = pluss::series::make({1.0, -2.0});
  const optim::OptimaState result = optim::optima(
      optim::problem().objective_analytic([&](const Pack &p) { const pluss::Series d = p.at("x") - c; return std::pair{d.square().sum(), optim::pack_series(2.0 * d)}; }),
      pluss::series::make({0.0, 0.0}));
  out.writeln("x =", result.params.at("x").items<double>());
}
```

Output

```text
x = [1.0000, -2.0000]
```

### Example 2

Two blocks: the gradient pack names both. Minimise `a² + 3b²` over `a + b = 1` (a linear row).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const Pack start = optim::pack_scalars({{"a", pluss::make_series(0.0)}, {"b", pluss::make_series(0.0)}});
  const optim::OptimaState result = optim::optima(
      optim::problem()
          .objective_analytic([](const Pack &p) {
            const pluss::Series a = p.at("a"), b = p.at("b");
            return std::pair{a.square() + 3.0 * b.square(), optim::pack_scalars({{"a", 2.0 * a.sum()}, {"b", 6.0 * b.sum()}})};
          })
          .ln_eq(pluss::series::make({{1.0, 1.0}}), pluss::series::make({1.0})),
      start);
  out.writeln("a =", result.params.at("a").items<double>(), " b =", result.params.at("b").items<double>());
}
```

Output

```text
a = [0.7500]  b = [0.2500]
```

### Example 3

Checking the gradient. A deliberately wrong gradient is caught by the audit.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const Pack at = optim::pack_series(pluss::series::make({1.0, 2.0}));
  const auto make = [](double scale) {
    return optim::problem().objective_analytic([scale](const Pack &p) { return std::pair{p.at("x").square().sum(), optim::pack_series(scale * p.at("x"))}; })
        .objective([](const Pack &p) { return p.at("x").square().sum(); }).build();
  };
  out.writeln("correct gradient  passes:", optim::audit_optima_derivatives(make(2.0), at).passed);
  out.writeln("wrong gradient    passes:", optim::audit_optima_derivatives(make(3.0), at).passed);
}
```

Output

```text
correct gradient  passes: true
wrong gradient    passes: false
```

## See Also

- [`objective`](objective.md): automatic differentiation
- [`ProblemBuilder`](Introduction.md): the builder
- [`optima`](../optima.md): solves the problem

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="objective.html">
    <span class="label">Previous</span>
    <span class="title">objective</span>
  </a>
  <a class="next-link" href="ubs.html">
    <span class="label">Next</span>
    <span class="title">ubs</span>
  </a>
</div>
<!-- apiref:nav:end -->
