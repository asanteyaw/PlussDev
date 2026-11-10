<!-- generated from solve.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::mpc::solve`

Solves a compiled or one-shot finite-horizon regulation problem.

## Signature

```cpp
control::mpc::Result control::mpc::solve(
    const control::mpc::Plan& plan,
    const pluss::Series& initial_state,
    const pluss::Series& reference,
    pluss::math::qp::WarmStart warm = {});

control::mpc::Result control::mpc::solve(
    const lti::StateSpace& system,
    const pluss::Series& initial_state,
    const pluss::Series& reference,
    const control::mpc::Weights& weights,
    const control::mpc::Bounds& bounds,
    control::mpc::Options options = {},
    pluss::math::qp::WarmStart warm = {});
```

## What It Does

The plan overload reuses compiled matrices and factorization. The one-shot overload coordinates `build` followed by the same solve kernel, making it convenient for isolated calculations but less efficient for repeated control updates.

## Parameters

`plan`
: A compiled problem from `control::mpc::build`.

`system`
: Discrete model used by the one-shot overload.

`initial_state`
: Current state vector.

`reference`
: A constant state target or a horizon-by-state reference trajectory.

`weights`
: Quadratic penalties for the one-shot overload.

`bounds`
: Input and optional state bounds for the one-shot overload.

`options`
: Horizon and QP settings for the one-shot overload.

`warm`
: Optional previous primal/dual QP state for faster repeated solves.

## Returns

An `mpc::Result`; `control` is the receding-horizon action while `controls` and `states` expose the full plan.

## Errors

Throws for shape or platform mismatches, invalid references, failed constraints, or QP non-convergence. The one-shot overload also performs all `build` validation.

## Interpretation and Use

Use the plan overload for repeated control updates; it avoids rebuilding invariant matrices. Use the one-shot overload for isolated analyses or prototyping. A warm start should come from a compatible previous QP result and improves speed without changing the mathematical problem.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Run the complete one-shot solve.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace control = pluss::ma::eng::control;
namespace lti = pluss::ma::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const lti::StateSpace plant = lti::state(
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}),
      1.0);
  const control::mpc::Weights weights{
      pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({0.1}, {1, 1}),
      pluss::make_series({2.0}, {1, 1})};
  const control::mpc::Bounds bounds{
      pluss::make_series({-0.4}), pluss::make_series({0.4}),
      pluss::make_series({-2.0}), pluss::make_series({1.05})};
  control::mpc::Options options;
  options.horizon = 4;
  options.qp.abs_tol = 1e-7;
  options.qp.rel_tol = 1e-7;
  const control::mpc::Result result = control::mpc::solve(
      plant, pluss::make_series({0.0}), pluss::make_series({1.0}),
      weights, bounds, options);

  out.writeln("first control =", result.control.items<double>());
  out.writeln("controls =", result.controls.flatten().items<double>());
  out.writeln("states =", result.states.flatten().items<double>());
  out.writeln("converged =", result.optimization.converged);
}
```

Output

```text
first control = [0.400000]
controls = [0.400000, 0.400000, 0.183267, 0.015936]
states = [0.400000, 0.800000, 0.983267, 0.999203]
converged = true
```
### Example 2

Reuse a plan and feed the previous optimizer state back as a warm start.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <iostream>

namespace control = pluss::ma::eng::control;
namespace lti = pluss::ma::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const lti::StateSpace plant = lti::state(
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}),
      1.0);
  const control::mpc::Weights weights{
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.1}, {1, 1})};
  const control::mpc::Bounds bounds{
      pluss::make_series({-0.5}), pluss::make_series({0.5})};
  const control::mpc::Plan plan =
      control::mpc::build(plant, weights, bounds, {.horizon = 3});
  const control::mpc::Result first = control::mpc::solve(
      plan, pluss::make_series({0.0}), pluss::make_series({1.0}));
  const control::mpc::Result warm = control::mpc::solve(
      plan, pluss::make_series({0.1}), pluss::make_series({1.0}),
      first.optimization.warm());

  std::cout << "horizon = " << plan.horizon() << ", states = " << plan.states()
            << ", inputs = " << plan.inputs() << "\n";
  out.writeln("cold iterations =", first.optimization.iterations);
  out.writeln("warm iterations =", warm.optimization.iterations);
  out.writeln("warm controls =", warm.controls.flatten().items<double>());
}
```

Output

```text
horizon = 3, states = 1, inputs = 1
cold iterations = 20
warm iterations = 20
warm controls = [0.500000, 0.366416, 0.030529]
```
### Example 3

Solve with non-default bounds, horizon, and tolerances.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace control = pluss::ma::eng::control;
namespace lti = pluss::ma::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const lti::StateSpace plant = lti::state(
      pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({0.0}, {1, 1}), 1.0);
  const control::mpc::Weights weights{
      pluss::make_series({2.0}, {1, 1}),
      pluss::make_series({0.5}, {1, 1})};
  const control::mpc::Bounds bounds{
      pluss::make_series({-0.25}), pluss::make_series({0.25})};
  control::mpc::Options options;
  options.horizon = 3;
  options.qp.max_iters = 1000;
  options.qp.abs_tol = 1e-7;
  options.qp.rel_tol = 1e-7;

  const control::mpc::Result result = control::mpc::solve(
      plant, pluss::make_series({0.0}), pluss::make_series({1.0}),
      weights, bounds, options);

  out.writeln("bounded controls =", result.controls.flatten().items<double>());
  out.writeln("predicted states =", result.states.flatten().items<double>());
  out.writeln("iterations =", result.optimization.iterations);
}
```

Output

```text
bounded controls = [0.250000, 0.250000, 0.250000]
predicted states = [0.250000, 0.500000, 0.750000]
iterations = 40
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="build.html">
    <span class="label">Previous</span>
    <span class="title">build</span>
  </a>
</div>
<!-- apiref:nav:end -->
