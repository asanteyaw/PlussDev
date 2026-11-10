<!-- generated from lqe.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::lqe`

Designs a steady-state linear quadratic estimator.

## Signature

```cpp
control::LqeResult control::lqe(
    const lti::StateSpace& system,
    const pluss::Series& process_cov,
    const pluss::Series& noise_cov,
    pluss::ma::linearalg::MatrixEqOptions options = {});

control::LqeResult control::lqe(
    const lti::StateSpace& system,
    const pluss::Series& process_cov,
    const pluss::Series& noise_cov,
    const pluss::Series& cross,
    pluss::ma::linearalg::MatrixEqOptions options = {});
```

## What It Does

`lqe` applies Riccati duality to estimator design. The model timebase selects continuous or discrete equations while preserving the state-space batch contract.

## Parameters

`system`
: The continuous or discrete plant whose state is estimated.

`process_cov`
: Process-noise covariance in state coordinates.

`noise_cov`
: Measurement-noise covariance in output coordinates.

`cross`
: Optional process/measurement cross covariance.

`options`
: Matrix-equation numerical controls.

## Returns

An `LqeResult` with covariance, gain, and estimator error dynamics.

## Errors

Throws for incompatible dimensions, invalid covariance matrices, or a failed stabilizing Riccati solve.

## Interpretation and Use

`lqe` is a design calculation, not a trajectory filter. It produces a steady-state gain from a model and noise covariances. Use `kalman` when actual observations must be processed, and use `smooth` when future observations may refine earlier states.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Design a continuous steady-state estimator.

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
      pluss::make_series({-1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({0.0}, {1, 1}));
  const control::LqeResult result = control::lqe(
      plant, pluss::make_series({0.25}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}));

  out.writeln("error covariance =", result.covariance.flatten().items<double>());
  out.writeln("estimator gain =", result.gain.flatten().items<double>());
  out.writeln("error pole =", result.closed_loop.flatten().items<double>());
}
```

Output

```text
error covariance = [0.118034]
estimator gain = [0.118034]
error pole = [-1.118034]
```
### Example 2

Reject a singular measurement covariance.

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
  const lti::StateSpace continuous = lti::state(
      pluss::make_series({-1.0}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}));
  try {
    static_cast<void>(control::kalman(
        continuous, pluss::make_series({1.0}, {1, 1}),
        pluss::make_series({0.1}, {1, 1}),
        pluss::make_series({0.2}, {1, 1}), pluss::make_series({0.0}),
        pluss::make_series({1.0}, {1, 1})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("continuous model rejected =",
                message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(control::lqe(
        continuous, pluss::make_series({1.0}, {1, 1}),
        pluss::make_series({0.0}, {1, 1})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("singular noise covariance rejected =",
                message.substr(0, message.find('\n')));
  }
}
```

Output

```text
continuous model rejected = eng::control::kalman: the state-space model must be discrete
singular noise covariance rejected = eng::control::lqe: noise_cov must be positive definite
```
### Example 3

Contrast the design result with finite-trajectory filtering.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace control = pluss::ma::eng::control;
namespace lti = pluss::ma::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const lti::StateSpace model = lti::state(
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.5}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({2.0}, {1, 1}),
      0.1);
  const control::KalmanResult result = control::kalman(
      model, pluss::make_series({2.0, 2.0, 2.0}, {3, 1}),
      pluss::make_series({0.0}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({0.0}), pluss::make_series({1.0}, {1, 1}),
      {.jitter = 0.0});

  out.writeln("filtered mean =", result.filtered_mean.flatten().items<double>());
  out.writeln("predicted observation =",
              result.predicted_obs.flatten().items<double>());
  out.writeln("log likelihood =", result.loglik.item<double>());
}
```

Output

```text
filtered mean = [1.000000, 1.333333, 1.500000]
predicted observation = [0.000000, 1.000000, 1.333333]
log likelihood = -4.949963
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="SmoothResult.html">
    <span class="label">Previous</span>
    <span class="title">SmoothResult</span>
  </a>
  <a class="next-link" href="kalman.html">
    <span class="label">Next</span>
    <span class="title">kalman</span>
  </a>
</div>
<!-- apiref:nav:end -->
