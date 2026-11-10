<!-- generated from evaluate_problem.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::evaluate_problem`

Evaluates a problem at a point: objective, gradient, constraint residuals and Jacobians, and the violation.

## Signature

```cpp
optim::ProblemEvaluation optim::evaluate_problem(const optim::ConstrainedProblem &problem, const optim::ParameterPack<pluss::Series> &params);
```

## What It Does

`evaluate_problem` is what the solvers call at every point, exposed. It returns a [`ProblemEvaluation`](ProblemEvaluation.md) with `value`, `gradient` (a pack with the parameters' layout), the residuals of every constraint family, their Jacobians, the largest violation, the first-order optimality measure and `feasible`. Use it to inspect a starting point, to check a problem before solving, or to compute the value of a candidate answer. It evaluates at the values only, so a point carrying earlier arithmetic history is detached first. A non-finite objective or constraint sets `status` and `message` rather than throwing.

## Parameters

`problem`
: The problem.

`params`
: The point, with the layout of the problem's unknowns.

## Returns

A `ProblemEvaluation`.

## Practical Examples

### Example 1

The value and gradient at a point.

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
  const optim::ConstrainedProblem problem =
      optim::problem()
          .objective([&](const Pack &p) { return (p.at("x") - target).square().sum(); })
          .nl_ineq([](const Pack &p) { return p.at("x").square().sum() - 1.0; });

  const optim::ProblemEvaluation e = optim::evaluate_problem(problem, optim::pack_series(pluss::series::make({0.0, 0.0})));
  out.writeln("value =", e.value.item<double>(), ", gradient =", e.gradient.at("x").items<double>());
}
```

Output

```text
value = 5.0000 , gradient = [-2.0000, -4.0000]
```

### Example 2

Feasibility of a point. The point `(1, 1)` is outside the unit disc: the violation is `|x|² − 1 = 1`.

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
  const optim::ConstrainedProblem problem =
      optim::problem()
          .objective([&](const Pack &p) { return (p.at("x") - target).square().sum(); })
          .nl_ineq([](const Pack &p) { return p.at("x").square().sum() - 1.0; });

  const optim::ProblemEvaluation e = optim::evaluate_problem(problem, optim::pack_series(pluss::series::make({1.0, 1.0})));
  out.writeln("feasible =", e.feasible, ", violation =", e.max_constraint_violation, ", inequality residual =", e.nonlinear_inequality_residual.item<double>());
}
```

Output

```text
feasible = false , violation = 1.0000 , inequality residual = 1.0000
```

### Example 3

The Jacobian of the constraint, `d(|x|² − 1)/dx = 2x`.

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
  const optim::ConstrainedProblem problem =
      optim::problem()
          .objective([&](const Pack &p) { return (p.at("x") - target).square().sum(); })
          .nl_ineq([](const Pack &p) { return p.at("x").square().sum() - 1.0; });

  const optim::ProblemEvaluation e = optim::evaluate_problem(problem, optim::pack_series(pluss::series::make({0.3, 0.4})));
  out.writeln("inequality Jacobian =", e.inequality_jacobian.at("x").reshape({2}).items<double>());
}
```

Output

```text
inequality Jacobian = [0.6000, 0.8000]
```

## See Also

- [`ProblemEvaluation`](ProblemEvaluation.md): its fields
- [`audit_optima_derivatives`](audit_optima_derivatives.md): checks derivatives

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="constraints.html">
    <span class="label">Previous</span>
    <span class="title">constraints</span>
  </a>
  <a class="next-link" href="optima.html">
    <span class="label">Next</span>
    <span class="title">optima</span>
  </a>
</div>
<!-- apiref:nav:end -->
