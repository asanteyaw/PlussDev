<!-- generated from kalman.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::kalman`

Runs a discrete linear Kalman filter.

## Signature

```cpp
control::KalmanResult control::kalman(
    const lti::StateSpace& system,
    const pluss::Series& observations,
    const pluss::Series& process_cov,
    const pluss::Series& noise_cov,
    const pluss::Series& initial_mean,
    const pluss::Series& initial_cov,
    control::KalmanOptions options = {});

control::KalmanResult control::kalman(
    const lti::StateSpace& system,
    const pluss::Series& observations,
    const pluss::Series& input,
    const pluss::Series& process_cov,
    const pluss::Series& noise_cov,
    const pluss::Series& initial_mean,
    const pluss::Series& initial_cov,
    control::KalmanOptions options = {});
```

## What It Does

The no-input overload supplies a zero input trajectory. The explicit-input overload includes known controls through both state and observation equations. Missing observations are handled through the shared information-mask machinery.

## Parameters

`system`
: A discrete state-space model. Filtering advances one model step per observation.

`observations`
: Observation trajectory with time on its penultimate structural axis.

`input`
: Optional known input trajectory aligned to the observations.

`process_cov`
: Process-noise covariance `Q`.

`noise_cov`
: Measurement-noise covariance `R`.

`initial_mean`
: Prior state mean before the first prediction/update.

`initial_cov`
: Prior state covariance.

`options`
: Information threshold and covariance jitter controls.

## Returns

A `KalmanResult` containing the complete forward trajectory and likelihood diagnostics.

## Errors

Throws when the model is continuous, dimensions do not align, covariance inputs are invalid, or numerical innovation solves fail.

## Interpretation and Use

The no-input overload is for autonomous or already-detrended systems. Use the explicit-input overload whenever known controls affect state or output equations; omitting them biases innovations and likelihood. Filtering is discrete even when the physical source model began in continuous time.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Run the no-input overload and inspect its likelihood.

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

Use the same forward model as the first stage of smoothing.

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

Reject a continuous model and invalid covariance data.

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
  <a class="prev-link" href="lqe.html">
    <span class="label">Previous</span>
    <span class="title">lqe</span>
  </a>
  <a class="next-link" href="smooth.html">
    <span class="label">Next</span>
    <span class="title">smooth</span>
  </a>
</div>
<!-- apiref:nav:end -->
