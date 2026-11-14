<!-- generated from KalmanOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::KalmanOptions`

Numerical controls shared by filtering and smoothing.

## Signature

```cpp
control::KalmanOptions
```

## What It Does

This value object groups artifacts that belong to one control calculation so callers can preserve both the primary answer and the diagnostics needed to interpret it. Its `Series` members retain the computation's dtype, platform, and leading batch dimensions.

`KalmanOptions` is a public alias of the shared linear-state filtering options; control filtering reuses that tested lower engine.

## Fields

### `info_tol`

Information threshold used when deciding whether an observation contributes. The default `0.0` keeps every finite informative observation.

### `jitter`

Non-negative diagonal covariance regularizer. The default is `1e-12`; increase it cautiously when innovation systems are nearly singular.

## Returns

This type is returned by, or supplied to, the functions described in this branch. Reading its fields does not trigger another solve.

## Errors

The value object performs no deferred validation. Invalid dimensions, domains, or numerical conditions are rejected by the function that creates or consumes it.

## Interpretation and Use

Leave `info_tol` at zero unless tiny information contributions must be ignored deliberately. `jitter` regularizes innovation covariance solves; increasing it trades a small covariance perturbation for numerical stability. It should not be used to conceal a structurally singular model.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Disable jitter explicitly on a well-conditioned scalar filter.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;
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
### Example 2

Reuse the same options contract through the smoother.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const lti::StateSpace model = lti::state(
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}),
      1.0);
  const control::SmoothResult result = control::smooth(
      model, pluss::make_series({0.2, 0.1, -0.3, 0.4}, {4, 1}),
      pluss::make_series({0.04}, {1, 1}),
      pluss::make_series({0.25}, {1, 1}), pluss::make_series({0.0}),
      pluss::make_series({1.0}, {1, 1}), {.jitter = 0.0});

  out.writeln("filtered mean =",
              result.filter.filtered_mean.flatten().items<double>());
  out.writeln("smoothed mean =", result.smoothed_mean.flatten().items<double>());
  out.writeln("final covariance =",
              result.smoothed_cov.select(-3, 3).flatten().items<double>());
}
```

Output

```text
filtered mean = [0.161240, 0.131147, -0.038921, 0.117636]
smoothed mean = [0.099188, 0.086873, 0.072458, 0.117636]
final covariance = [0.089172]
```
### Example 3

See structural errors that option tuning must not mask.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="LqeResult.html">
    <span class="label">Previous</span>
    <span class="title">LqeResult</span>
  </a>
  <a class="next-link" href="KalmanResult.html">
    <span class="label">Next</span>
    <span class="title">KalmanResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
