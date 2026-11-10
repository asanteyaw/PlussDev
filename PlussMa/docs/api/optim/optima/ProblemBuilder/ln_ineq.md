<!-- generated from ln_ineq.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 876b692acb7b -->

# `ProblemBuilder::ln_ineq`

[Back to `optim::ProblemBuilder`](../ProblemBuilder.md)

Adds the linear inequalities `Aineq x ≤ bineq`.

## Signature

```cpp
optim::ProblemBuilder &optim::ProblemBuilder::ln_ineq(const pluss::Series &Aineq, const pluss::Series &bineq);
```

## What It Does

`Aineq` has one row per inequality and one column per unknown scalar; the row `a` requires `a·x ≤ b`. To require `x₀ ≥ 1`, write the row `(−1, 0)` with right-hand side `−1`. Rows may mix with bounds, equalities and nonlinear constraints.

## Parameters

`Aineq`
: Matrix, `[rows, unknowns]`.

`bineq`
: Right-hand side, `[rows]`.

## Returns

The same builder, for chaining.

## Practical Examples

### Example 1

A halfspace. The nearest point to `(2,2)` with `x₀ + x₁ ≤ 3`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const optim::OptimaState result = optim::optima(optim::problem().objective([](const Pack &p) { return (p.at("x") - 2.0).square().sum(); })
      .ln_ineq(pluss::series::make({{1.0, 1.0}}), pluss::series::make({3.0})), pluss::series::make({0.0, 0.0}));
  out.writeln("x =", result.params.at("x").items<double>());
}
```

Output

```text
x = [1.5000, 1.5000]
```

### Example 2

Two rows bound a wedge: `x₀ ≤ 1` and `x₁ ≤ 0.5`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series target = pluss::series::make({1.0, 2.0});

  const optim::OptimaState result = optim::optima(optim::problem().objective([&](const Pack &p) { return (p.at("x") - target).square().sum(); })
      .ln_ineq(pluss::series::make({{1.0, 0.0}, {0.0, 1.0}}), pluss::series::make({1.0, 0.5})), pluss::series::make({0.0, 0.0}));
  out.writeln("x =", result.params.at("x").items<double>());
}
```

Output

```text
x = [1.0000, 0.5000]
```

### Example 3

A lower bound written as a row: `x₀ ≥ 1` becomes `−x₀ ≤ −1`.

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
      .ln_ineq(pluss::series::make({{-1.0, 0.0}}), pluss::series::make({-1.0})), pluss::series::make({2.0, 2.0}));
  out.writeln("x =", result.params.at("x").items<double>());
}
```

Output

```text
x = [1.0000, 0.0000]
```

## See Also

- [`ln_eq`](ln_eq.md): linear equalities
- [`ProblemBuilder`](Introduction.md): the builder
- [`optima`](../optima.md): solves the problem

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ln_eq.html">
    <span class="label">Previous</span>
    <span class="title">ln_eq</span>
  </a>
  <a class="next-link" href="nl_eq.html">
    <span class="label">Next</span>
    <span class="title">nl_eq</span>
  </a>
</div>
<!-- apiref:nav:end -->
