<!-- generated from Result.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::mpc::Result`

The selected action, complete plan, state trajectory, and optimizer diagnostics.

## Signature

```cpp
control::mpc::Result
```

## What It Does

This value object groups artifacts that belong to one control calculation so callers can preserve both the primary answer and the diagnostics needed to interpret it. Its `Series` members retain the computation's dtype, platform, and leading batch dimensions.



## Fields

### `control`

The first control action, ready to apply in receding-horizon operation.

### `controls`

The full optimized control sequence over the horizon.

### `states`

The predicted state sequence after applying the optimized controls.

### `optimization`

The underlying `pluss::math::qp::Result`, including convergence and residual diagnostics.

## Returns

This type is returned by, or supplied to, the functions described in this branch. Reading its fields does not trigger another solve.

## Errors

The value object performs no deferred validation. Invalid dimensions, domains, or numerical conditions are rejected by the function that creates or consumes it.

## Interpretation and Use

Apply only `control`, the first action, in receding-horizon operation. `controls` and `states` are predictions used for inspection and warm-start planning, not commitments. Always check `optimization.converged` and residual diagnostics before applying the action.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Inspect the first action, full input plan, states, and convergence.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;
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

Observe saturation and optimizer iterations under tighter bounds.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;
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
### Example 3

Compare cold and warm-started result diagnostics.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <iostream>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;
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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Options.html">
    <span class="label">Previous</span>
    <span class="title">Options</span>
  </a>
  <a class="next-link" href="Plan.html">
    <span class="label">Next</span>
    <span class="title">Plan</span>
  </a>
</div>
<!-- apiref:nav:end -->
