<!-- generated from smooth.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::smooth`

Runs forward filtering followed by backward smoothing.

## Signature

```cpp
control::SmoothResult control::smooth(
    const lti::StateSpace& system,
    const pluss::Series& observations,
    const pluss::Series& process_cov,
    const pluss::Series& noise_cov,
    const pluss::Series& initial_mean,
    const pluss::Series& initial_cov,
    control::KalmanOptions options = {});

control::SmoothResult control::smooth(
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

`smooth` first delegates to the forward filter and then conditions each earlier state on all later observations. Smoothed estimates can therefore differ from filtered estimates even though both use the same model and options.

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

A `SmoothResult` containing the forward filter and the Rauch-Tung-Striebel backward trajectory.

## Errors

Throws under the same model, shape, covariance, and numerical conditions as `kalman`.

## Interpretation and Use

Smoothing performs a complete forward filter before the backward pass, so its memory cost includes trajectory covariances. Use it only when the full observation block is available. For streaming or causal decisions, retain `kalman` instead.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Run the smoother and inspect its retrospective correction.

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
### Example 2

Compare against the causal forward-filter output.

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
### Example 3

Review validation inherited from the forward pass.

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
  <a class="prev-link" href="kalman.html">
    <span class="label">Previous</span>
    <span class="title">kalman</span>
  </a>
</div>
<!-- apiref:nav:end -->
