<!-- generated from build.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::mpc::build`

Compiles a reusable finite-horizon MPC plan.

## Signature

```cpp
control::mpc::Plan control::mpc::build(
    const lti::StateSpace& system,
    const control::mpc::Weights& weights,
    const control::mpc::Bounds& bounds,
    control::mpc::Options options = {});
```

## What It Does

`build` performs work that does not depend on the current state or reference: prediction assembly, cost condensation, constraint assembly, and solver factorization. Reusing the result is the preferred receding-horizon path.

## Parameters

`system`
: A discrete state-space model.

`weights`
: State, input, and optional terminal quadratic weights.

`bounds`
: Input and optional state bounds.

`options`
: Prediction horizon and QP settings.

## Returns

A reusable `control::mpc::Plan` with condensed matrices and factorization.

## Errors

Throws for continuous models, non-positive horizons, invalid weights, inconsistent bounds, or QP preparation failures.

## Interpretation and Use

Call `build` outside the real-time update loop. It validates the discrete model, expands horizon matrices, assembles constraints, and prepares the QP solver once. The resulting plan preserves dtype and platform from its inputs.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Build once and solve twice.

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
### Example 2

Compile non-default horizon and QP settings.

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
### Example 3

See build-time rejection of continuous models and zero horizons.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>
#include <string>

namespace control = pluss::ma::eng::control;
namespace lti = pluss::ma::eng::lti;
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
  <a class="prev-link" href="Plan.html">
    <span class="label">Previous</span>
    <span class="title">Plan</span>
  </a>
  <a class="next-link" href="solve.html">
    <span class="label">Next</span>
    <span class="title">solve</span>
  </a>
</div>
<!-- apiref:nav:end -->
