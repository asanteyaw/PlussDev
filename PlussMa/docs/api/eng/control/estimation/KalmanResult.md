<!-- generated from KalmanResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::KalmanResult`

The full forward Kalman-filter trajectory and likelihood record.

## Signature

```cpp
control::KalmanResult
```

## What It Does

This value object groups artifacts that belong to one control calculation so callers can preserve both the primary answer and the diagnostics needed to interpret it. Its `Series` members retain the computation's dtype, platform, and leading batch dimensions.



## Fields

### `filtered_mean`

Posterior state means after each observation update.

### `filtered_cov`

Posterior state covariances after each observation update.

### `predicted_mean`

Prior state means before each update.

### `predicted_cov`

Prior state covariances before each update.

### `predicted_obs`

Predicted observations `C x + D u`.

### `innovation`

Observation residuals relative to `predicted_obs`.

### `innovation_cov`

Innovation covariance matrices.

### `gain`

Time-varying Kalman gains.

### `data_used`

Mask recording which observed values participated in each update.

### `loglik_terms`

Per-step Gaussian log-likelihood contributions.

### `loglik`

The accumulated log likelihood over the trajectory.

## Returns

This type is returned by, or supplied to, the functions described in this branch. Reading its fields does not trigger another solve.

## Errors

The value object performs no deferred validation. Invalid dimensions, domains, or numerical conditions are rejected by the function that creates or consumes it.

## Interpretation and Use

Use prior fields (`predicted_*`) to evaluate forecasts and posterior fields (`filtered_*`) to evaluate updated state knowledge. `innovation` and `innovation_cov` support residual diagnostics; `loglik_terms` localize poor fit while `loglik` supports whole-run comparison.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Inspect posterior means, predicted observations, and total likelihood.

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
### Example 2

Pass the forward result into the smoothing interpretation.

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

See which invalid models fail before a result can be produced.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="KalmanOptions.html">
    <span class="label">Previous</span>
    <span class="title">KalmanOptions</span>
  </a>
  <a class="next-link" href="SmoothResult.html">
    <span class="label">Next</span>
    <span class="title">SmoothResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
