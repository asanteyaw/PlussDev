<!-- generated from constraints.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::constraints`

Starts a builder for the constraints of a problem, to be passed to `optima` alongside a separately written objective.

## Signature

```cpp
optim::ObjectiveConstraintsBuilder optim::constraints();
```

## What It Does

`constraints()` returns an `ObjectiveConstraintsBuilder` with the same constraint methods as [`ProblemBuilder`](ProblemBuilder/Introduction.md) (`lbs`, `ubs`, `ln_eq`, `ln_ineq`, `nl_eq`, `nl_ineq`, `nonlinear_equality`, `nonlinear_inequality`, `nonlinear_analytic`) but no objective. It is the fourth argument of the objective-first forms of [`optima`](optima.md), which keeps a small problem on one line: objective, start, options, constraints.

## Returns

An empty `ObjectiveConstraintsBuilder`; it converts to `ObjectiveConstraints` implicitly.

## Practical Examples

### Example 1

A bound constraint. Minimise the distance to `(3,−2)` inside the box `[0,2]²`: the nearest corner.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series target = pluss::series::make({3.0, -2.0});
  const optim::OptimaState result = optim::optima([&](const pluss::Series &x) { return (x - target).square().sum(); }, pluss::series::make({1.0, 1.0}), {},
      optim::constraints().lbs(pluss::series::make({0.0, 0.0})).ubs(pluss::series::make({2.0, 2.0})));
  out.writeln("x =", result.params.at("x").items<double>());
}
```

Output

```text
x = [2.0000, 0.0000]
```

### Example 2

A nonlinear equality. The nearest point to `(3,3)` on the unit circle.

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
  const optim::OptimaState result = optim::optima([&](const pluss::Series &x) { return (x - target).square().sum(); }, pluss::series::make({1.0, 0.0}), {},
      optim::constraints().nl_eq([](const Pack &p) { return p.at("x").square().sum() - 1.0; }));
  out.writeln("x =", result.params.at("x").items<double>());
}
```

Output

```text
x = [0.7071, 0.7071]
```

### Example 3

A constraint set is a value, reusable across objectives.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto box = optim::constraints().lbs(pluss::series::make({0.0})).ubs(pluss::series::make({1.0}));
  for (const double c : {-1.0, 0.4, 3.0}) {
    const optim::OptimaState result = optim::optima([&](const pluss::Series &x) { return (x - c).square().sum(); }, pluss::series::make({0.5}), {}, box);
    out.writeln("target", c, "-> x =", result.params.at("x").items<double>());
  }
}
```

Output

```text
target -1.0000 -> x = [0.0000]
target 0.4000 -> x = [0.4000]
target 3.0000 -> x = [1.0000]
```

## See Also

- [`optima`](optima.md): takes the constraints
- [`problem`](problem.md): objective and constraints together

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="audit_optima_derivatives.html">
    <span class="label">Previous</span>
    <span class="title">audit_optima_derivatives</span>
  </a>
  <a class="next-link" href="evaluate_problem.html">
    <span class="label">Next</span>
    <span class="title">evaluate_problem</span>
  </a>
</div>
<!-- apiref:nav:end -->
