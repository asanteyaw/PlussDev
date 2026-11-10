<!-- generated from mpc.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Model-predictive control

The `control::mpc` branch solves condensed finite-horizon regulation for a discrete linear system. State and input tracking penalties define the objective, while input bounds and optional state bounds define hard trajectory constraints.

## Compile Once, Solve Repeatedly

`build` assembles prediction matrices, condenses the dynamics, and factorizes the invariant quadratic program. Reuse the returned `Plan` when the model, weights, bounds, and horizon remain fixed but initial state or reference changes. The one-shot `solve` overload builds and solves in one call for occasional use.

## Practical Examples

The examples solve a bounded regulation problem, reuse a compiled plan with a warm start, and demonstrate timebase and horizon validation.

### Example 1

Solve a constrained scalar tracking problem and inspect the planned trajectories.

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

Compile a reusable plan and warm-start the next receding-horizon solve.

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

Reject a continuous plant and a non-positive prediction horizon.

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
```{toctree}
:maxdepth: 2
:hidden:

mpc/Weights
mpc/Bounds
mpc/Options
mpc/Result
mpc/Plan
mpc/build
mpc/solve
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

mpc/Weights
mpc/Bounds
mpc/Options
mpc/Result
mpc/Plan
mpc/build
mpc/solve
```
<!-- apiref:toctree:end -->
