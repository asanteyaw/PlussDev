<!-- generated from Bounds.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::mpc::Bounds`

Input and optional state bounds over the prediction horizon.

## Signature

```cpp
control::mpc::Bounds
```

## What It Does

This value object groups artifacts that belong to one control calculation so callers can preserve both the primary answer and the diagnostics needed to interpret it. Its `Series` members retain the computation's dtype, platform, and leading batch dimensions.



## Fields

### `input_lower`

Required lower input limits, broadcast across or specified over the horizon.

### `input_upper`

Required upper input limits matching `input_lower`.

### `state_lower`

Optional lower state limits; leave undefined with `state_upper` to disable state constraints.

### `state_upper`

Optional upper state limits paired with `state_lower`.

## Returns

This type is returned by, or supplied to, the functions described in this branch. Reading its fields does not trigger another solve.

## Errors

The value object performs no deferred validation. Invalid dimensions, domains, or numerical conditions are rejected by the function that creates or consumes it.

## Interpretation and Use

Input bounds are mandatory and may be constant or horizon-shaped. State bounds are optional but must be supplied as a lower/upper pair. Tight bounds can make the QP infeasible; inspect optimizer convergence rather than assuming a returned trajectory is usable.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Apply only input bounds and observe saturation.

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
### Example 2

Add state bounds to the same regulation problem.

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
### Example 3

Review plan validation before constrained solving.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>
#include <string>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const control::mpc::Weights weights{
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.1}, {1, 1})};
  const control::mpc::Bounds bounds{
      pluss::make_series({-0.5}), pluss::make_series({0.5})};
  try {
    const lti::StateSpace continuous = lti::state(
        pluss::make_series({-1.0}, {1, 1}),
        pluss::make_series({1.0}, {1, 1}),
        pluss::make_series({1.0}, {1, 1}),
        pluss::make_series({0.0}, {1, 1}));
    static_cast<void>(control::mpc::build(continuous, weights, bounds));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("continuous plant rejected =",
                message.substr(0, message.find('\n')));
  }
  try {
    const lti::StateSpace discrete = lti::state(
        pluss::make_series({1.0}, {1, 1}),
        pluss::make_series({1.0}, {1, 1}),
        pluss::make_series({1.0}, {1, 1}),
        pluss::make_series({0.0}, {1, 1}), 1.0);
    static_cast<void>(control::mpc::build(
        discrete, weights, bounds, {.horizon = 0}));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("zero horizon rejected =", message.substr(0, message.find('\n')));
  }
}
```

Output

```text
continuous plant rejected = eng::control::mpc::solve: system must be discrete
zero horizon rejected = eng::control::mpc::solve: horizon must be positive
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Weights.html">
    <span class="label">Previous</span>
    <span class="title">Weights</span>
  </a>
  <a class="next-link" href="Options.html">
    <span class="label">Next</span>
    <span class="title">Options</span>
  </a>
</div>
<!-- apiref:nav:end -->
