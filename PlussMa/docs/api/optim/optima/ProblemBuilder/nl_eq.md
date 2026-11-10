<!-- generated from nl_eq.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ee9a4dc810e0 -->

# `ProblemBuilder::nl_eq`

[Back to `optim::ProblemBuilder`](../ProblemBuilder.md)

The short name of `nonlinear_equality`.

## Signature

```cpp
template <typename Fn> optim::ProblemBuilder &optim::ProblemBuilder::nl_eq(Fn &&fn);
```

## What It Does

Identical to [`nonlinear_equality`](nonlinear_equality.md): `fn` returns `h(x)` and the constraint is `h(x) = 0`.

## Parameters

`Fn &&fn`
: Returns `h(x)`.

## Returns

The same builder, for chaining.

## Practical Examples

### Example 1

A circle.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const optim::OptimaState result = optim::optima(optim::problem().objective([](const Pack &p) { return (p.at("x") - 3.0).square().sum(); })
      .nl_eq([](const Pack &p) { return p.at("x").square().sum() - 2.0; }), pluss::series::make({1.0, 0.5}));
  out.writeln("x =", result.params.at("x").items<double>());
}
```

Output

```text
x = [1.0000, 1.0000]
```

### Example 2

Combined with an inequality.

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
      .nl_eq([](const Pack &p) { return p.at("x").sum() - 2.0; }).nl_ineq([](const Pack &p) { return p.at("x").square().sum() - 4.0; }), pluss::series::make({0.0, 0.0}));
  out.writeln("x =", result.params.at("x").items<double>());
}
```

Output

```text
x = [1.0000, 1.0000]
```

### Example 3

A vector of two equalities in one callback: `x₀ + x₁ = 1` and `x₀ − x₁ = 0.2`.

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
      .nl_eq([](const Pack &p) {
        const pluss::Series rows = pluss::series::make({{1.0, 1.0}, {1.0, -1.0}});
        return rows.matmul(p.at("x").reshape({2, 1})).reshape({2}) - pluss::series::make({1.0, 0.2});
      }), pluss::series::make({0.0, 0.0}));
  out.writeln("x =", result.params.at("x").items<double>());
}
```

Output

```text
x = [0.6000, 0.4000]
```

## See Also

- [`nonlinear_equality`](nonlinear_equality.md): the long name
- [`ProblemBuilder`](Introduction.md): the builder
- [`optima`](../optima.md): solves the problem

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ln_ineq.html">
    <span class="label">Previous</span>
    <span class="title">ln_ineq</span>
  </a>
  <a class="next-link" href="nl_ineq.html">
    <span class="label">Next</span>
    <span class="title">nl_ineq</span>
  </a>
</div>
<!-- apiref:nav:end -->
