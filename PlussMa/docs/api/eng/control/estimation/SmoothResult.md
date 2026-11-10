<!-- generated from SmoothResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::SmoothResult`

Forward-filter output augmented by backward-smoothed state estimates.

## Signature

```cpp
control::SmoothResult
```

## What It Does

This value object groups artifacts that belong to one control calculation so callers can preserve both the primary answer and the diagnostics needed to interpret it. Its `Series` members retain the computation's dtype, platform, and leading batch dimensions.



## Fields

### `filter`

The complete `control::KalmanResult` from the forward pass.

### `smoothed_mean`

State means conditioned on the complete observation sequence.

### `smoothed_cov`

State covariances conditioned on the complete observation sequence.

### `gain`

Backward smoothing gains connecting adjacent time steps.

## Returns

This type is returned by, or supplied to, the functions described in this branch. Reading its fields does not trigger another solve.

## Errors

The value object performs no deferred validation. Invalid dimensions, domains, or numerical conditions are rejected by the function that creates or consumes it.

## Interpretation and Use

The embedded `filter` preserves the causal estimate, while `smoothed_mean` and `smoothed_cov` use future observations. Never substitute smoothed states in an online controller: they are retrospective. They are appropriate for reconstruction, parameter analysis, and offline diagnostics.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Compare filtered and smoothed trajectories directly.

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
### Example 2

Inspect the forward quantities retained inside a smooth result.

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
### Example 3

Review model and covariance failures shared with filtering.

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
  <a class="prev-link" href="KalmanResult.html">
    <span class="label">Previous</span>
    <span class="title">KalmanResult</span>
  </a>
  <a class="next-link" href="lqe.html">
    <span class="label">Next</span>
    <span class="title">lqe</span>
  </a>
</div>
<!-- apiref:nav:end -->
