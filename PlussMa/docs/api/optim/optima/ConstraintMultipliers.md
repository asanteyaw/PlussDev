<!-- generated from ConstraintMultipliers.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::ConstraintMultipliers`

The Lagrange multipliers of a solution, one vector per constraint family.

At a local minimum the gradient of the objective is balanced by the gradients of the active constraints; the weights are the multipliers. A multiplier measures how much the optimal objective would change per unit relaxation of its constraint: it is zero for an inactive inequality and positive for an active one. `linear_equalities`, `nonlinear_equalities`, `linear_inequalities`, `nonlinear_inequalities`, `lower_bounds` and `upper_bounds` hold one entry per row of that family, in the order the rows were given; a family that the problem does not have is an undefined `Series`. The sign follows the convention `∇f + Σ λ ∇c = 0`, so the multiplier of an equality can be negative while that of an active inequality `g ≤ 0` is positive.

## Fields

### `linear_equalities`

`pluss::Series`, default `undefined`.

Multipliers of the linear equalities, one per row.

### `nonlinear_equalities`

`pluss::Series`, default `undefined`.

Multipliers of the nonlinear equalities, one per row.

### `linear_inequalities`

`pluss::Series`, default `undefined`.

Multipliers of the linear inequalities, one per row.

### `nonlinear_inequalities`

`pluss::Series`, default `undefined`.

Multipliers of the nonlinear inequalities, one per row.

### `lower_bounds`

`pluss::Series`, default `undefined`.

Multipliers of the lower bounds, one per row.

### `upper_bounds`

`pluss::Series`, default `undefined`.

Multipliers of the upper bounds, one per row.

## Guidance

Read as `state.multipliers`.

## Practical Examples

### Example 1

A nonlinear inequality multiplier. At the answer on the circle the multiplier is positive.

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
  const auto objective = [&](const pluss::Series &x) { return (x - target).square().sum(); };
  const auto disc = optim::constraints().nl_ineq([](const Pack &p) { return p.at("x").square().sum() - 1.0; });
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), {}, disc);
  out.writeln("nonlinear inequality =", result.multipliers.nonlinear_inequalities.items<double>());
}
```

Output

```text
nonlinear inequality = [1.2361]
```

### Example 2

Bound multipliers. The upper bound `x₀ ≤ 0.5` is active (target 1); the other is not, so its multiplier is zero.

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

  const optim::OptimaState result = optim::optima([&](const pluss::Series &x) { return (x - target).square().sum(); }, pluss::series::make({0.0, 0.0}), {},
      optim::constraints().ubs(pluss::series::make({0.5, 5.0})));
  out.writeln("upper bounds =", result.multipliers.upper_bounds.items<double>());
}
```

Output

```text
upper bounds = [1.0000, 0.0000]
```

### Example 3

Linear rows. The equality `x₀ + x₁ = 1` has a multiplier, the price of moving its right-hand side.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const optim::OptimaState result = optim::optima([](const pluss::Series &x) { return x.square().sum(); }, pluss::series::make({0.0, 0.0}), {},
      optim::constraints().ln_eq(pluss::series::make({{1.0, 1.0}}), pluss::series::make({1.0})));
  out.writeln("linear equality =", result.multipliers.linear_equalities.items<double>());
}
```

Output

```text
linear equality = [-1.0000]
```

## See Also

- [`OptimaState`](OptimaState.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ConstrainedProblem.html">
    <span class="label">Previous</span>
    <span class="title">ConstrainedProblem</span>
  </a>
  <a class="next-link" href="DerivativeAuditReport.html">
    <span class="label">Next</span>
    <span class="title">DerivativeAuditReport</span>
  </a>
</div>
<!-- apiref:nav:end -->
