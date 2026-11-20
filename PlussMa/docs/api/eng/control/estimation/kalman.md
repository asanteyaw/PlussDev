<!-- generated from kalman.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::kalman`

Runs a Kalman filter over a sequence of observations.

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

For `x[k+1] = A x[k] + B u[k] + w[k]` and `y[k] = C x[k] + D u[k] + v[k]` with `w ~ N(0, Q)` and `v ~ N(0, R)`, it alternates a prediction from the model with an update from each observation and returns, at every sample, the filtered mean and covariance together with the prediction, the innovation, the gain and the log-likelihood. Samples whose observation is NaN, or whose innovation variance does not exceed `info_tol`, are skipped: the filter predicts through them and the covariance grows. An overload takes a known input `u`.

## Parameters

`system`
: A `StateSpace`, usually discrete (the filter steps once per observation).

`observations`
: `[time, outputs]`, one row per sample. A NaN entry marks a missing observation.

`process_cov`, `noise_cov`
: The covariances `Q` of the state disturbance and `R` of the measurement noise, symmetric and positive semidefinite.

`initial_mean`, `initial_cov`
: The mean and covariance of the state before the first observation.

`input`
: Optional `[time, inputs]` known input (second overload).

`options`
: `KalmanOptions`: the information threshold and the covariance jitter.

## Returns

A `KalmanResult` with one entry per sample.

## Errors

A covariance that is not positive semidefinite, observations of the wrong width, an initial covariance of the wrong size and invalid options throw `pluss::Error`.

## Interpretation and Use

The first example writes the scalar recursion by hand (predict `p + q`, gain `p / (p + r)`, update) and finds no difference from the library in the mean or covariance. The gain falls from 0.91 to 0.27 as the filter learns, settling on the steady-state gain that `lqe` computes (the second example). A NaN sample is skipped and the covariance rises from 0.376 to 0.476 before shrinking again (the third). Giving the known input, the fourth example tracks a ramp with an error of 0.19 against 0.89 without it.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The library against a hand-written scalar recursion.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }
static double max_abs(const pluss::Series &s) { return s.abs().max().item<double>(); }

// A double integrator x'' = u as the matrices of x' = A x + B u.
static pluss::Series A2() { return mat({0.0, 1.0, 0.0, 0.0}, 2, 2); }
static pluss::Series B2() { return mat({0.0, 1.0}, 2, 1); }

// A scalar random walk x[k+1] = x[k] + w, y[k] = x[k] + v, sampled every second.
static lti::StateSpace walk() { return lti::state(mat({1.0}, 1, 1), mat({0.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1), 1.0); }
// Eight observations of a constant 1 with a deterministic disturbance in place of noise.
static std::vector<double> readings(int n = 8) {
  std::vector<double> y;
  for (int k = 0; k < n; ++k) y.push_back(1.0 + 0.3 * std::sin(7.0 * k));
  return y;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // The scalar Kalman recursion written out by hand: predict p + q, gain k = p / (p + r), update.
  const auto y = readings();
  const ctl::KalmanResult r = ctl::kalman(walk(), mat(readings(), 8, 1), mat({0.1}, 1, 1), mat({1.0}, 1, 1), pluss::make_series({0.0}), mat({10.0}, 1, 1));
  const auto mean = r.filtered_mean.flatten().items<double>(), cov = r.filtered_cov.flatten().items<double>();
  double x = 0.0, p = 10.0, worst_mean = 0.0, worst_cov = 0.0;
  for (std::size_t k = 0; k < y.size(); ++k) {
    const double predicted = p + 0.1, gain = predicted / (predicted + 1.0);
    x += gain * (y[k] - x);
    p = (1.0 - gain) * predicted;
    worst_mean = std::max(worst_mean, std::abs(x - mean[k]));
    worst_cov = std::max(worst_cov, std::abs(p - cov[k]));
  }
  out.writeln("filtered mean =", mean);
  out.writeln("largest difference from the hand recursion: mean", worst_mean, " covariance", worst_cov);
}
```

Output

```text
filtered mean = [0.9099, 1.0542, 1.1456, 1.1796, 1.1504, 1.0711, 0.9751, 0.9035]
largest difference from the hand recursion: mean 0.0000  covariance 0.0000
```
### Example 2

The gain over time against the steady-state gain.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }
static double max_abs(const pluss::Series &s) { return s.abs().max().item<double>(); }

// A double integrator x'' = u as the matrices of x' = A x + B u.
static pluss::Series A2() { return mat({0.0, 1.0, 0.0, 0.0}, 2, 2); }
static pluss::Series B2() { return mat({0.0, 1.0}, 2, 1); }

// A scalar random walk x[k+1] = x[k] + w, y[k] = x[k] + v, sampled every second.
static lti::StateSpace walk() { return lti::state(mat({1.0}, 1, 1), mat({0.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1), 1.0); }
// Eight observations of a constant 1 with a deterministic disturbance in place of noise.
static std::vector<double> readings(int n = 8) {
  std::vector<double> y;
  for (int k = 0; k < n; ++k) y.push_back(1.0 + 0.3 * std::sin(7.0 * k));
  return y;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const ctl::KalmanResult r = ctl::kalman(walk(), mat(readings(), 8, 1), mat({0.1}, 1, 1), mat({1.0}, 1, 1), pluss::make_series({0.0}), mat({10.0}, 1, 1));
  const auto gain = r.gain.flatten().items<double>();
  out.writeln("Kalman gain over time =", gain);
  const ctl::LqeResult steady = ctl::lqe(walk(), mat({0.1}, 1, 1), mat({1.0}, 1, 1));
  out.writeln("steady-state gain from lqe =", steady.gain.flatten().items<double>(), "(the gain settles toward it)");
}
```

Output

```text
Kalman gain over time = [0.9099, 0.5025, 0.3760, 0.3225, 0.2970, 0.2842, 0.2776, 0.2741]
steady-state gain from lqe = [0.2702] (the gain settles toward it)
```
### Example 3

A missing observation.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }
static double max_abs(const pluss::Series &s) { return s.abs().max().item<double>(); }

// A double integrator x'' = u as the matrices of x' = A x + B u.
static pluss::Series A2() { return mat({0.0, 1.0, 0.0, 0.0}, 2, 2); }
static pluss::Series B2() { return mat({0.0, 1.0}, 2, 1); }

// A scalar random walk x[k+1] = x[k] + w, y[k] = x[k] + v, sampled every second.
static lti::StateSpace walk() { return lti::state(mat({1.0}, 1, 1), mat({0.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1), 1.0); }
// Eight observations of a constant 1 with a deterministic disturbance in place of noise.
static std::vector<double> readings(int n = 8) {
  std::vector<double> y;
  for (int k = 0; k < n; ++k) y.push_back(1.0 + 0.3 * std::sin(7.0 * k));
  return y;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  auto y = readings();
  y[3] = std::nan("");
  const ctl::KalmanResult r = ctl::kalman(walk(), mat(y, 8, 1), mat({0.1}, 1, 1), mat({1.0}, 1, 1), pluss::make_series({0.0}), mat({10.0}, 1, 1));
  out.writeln("a NaN at sample 3: data_used =", r.data_used.flatten().items<bool>());
  out.writeln("filtered covariance =", r.filtered_cov.flatten().items<double>(), "(it grows at the missing sample, then shrinks again)");
}
```

Output

```text
a NaN at sample 3: data_used = [true, true, true, false, true, true, true, true]
filtered covariance = [0.9099, 0.5025, 0.3760, 0.4760, 0.3655, 0.3176, 0.2946, 0.2829] (it grows at the missing sample, then shrinks again)
```
### Example 4

A known input improves the estimate of a state it drives.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }
static double max_abs(const pluss::Series &s) { return s.abs().max().item<double>(); }

// A double integrator x'' = u as the matrices of x' = A x + B u.
static pluss::Series A2() { return mat({0.0, 1.0, 0.0, 0.0}, 2, 2); }
static pluss::Series B2() { return mat({0.0, 1.0}, 2, 1); }

// A scalar random walk x[k+1] = x[k] + w, y[k] = x[k] + v, sampled every second.
static lti::StateSpace walk() { return lti::state(mat({1.0}, 1, 1), mat({0.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1), 1.0); }
// Eight observations of a constant 1 with a deterministic disturbance in place of noise.
static std::vector<double> readings(int n = 8) {
  std::vector<double> y;
  for (int k = 0; k < n; ++k) y.push_back(1.0 + 0.3 * std::sin(7.0 * k));
  return y;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // x[k+1] = x[k] + u[k]: the state climbs by a known 0.5 per step and the sensor sees it with a deterministic disturbance.
  const lti::StateSpace system = lti::state(mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1), 1.0);
  std::vector<double> y, truth;
  for (int k = 0; k < 8; ++k) { truth.push_back(0.5 * k); y.push_back(0.5 * k + 0.3 * std::sin(7.0 * k)); }
  const pluss::Series u = mat(std::vector<double>(8, 0.5), 8, 1);
  const ctl::KalmanResult with_input = ctl::kalman(system, mat(y, 8, 1), u, mat({0.01}, 1, 1), mat({1.0}, 1, 1), pluss::make_series({0.0}), mat({1.0}, 1, 1));
  const ctl::KalmanResult without = ctl::kalman(system, mat(y, 8, 1), mat({0.01}, 1, 1), mat({1.0}, 1, 1), pluss::make_series({0.0}), mat({1.0}, 1, 1));
  double e_with = 0.0, e_without = 0.0;
  const auto a = with_input.filtered_mean.flatten().items<double>(), b = without.filtered_mean.flatten().items<double>();
  for (std::size_t k = 0; k < 8; ++k) { e_with += std::abs(a[k] - truth[k]); e_without += std::abs(b[k] - truth[k]); }
  out.writeln("mean absolute error of the filtered mean: with the input", e_with / 8.0, " without it", e_without / 8.0);
}
```

Output

```text
mean absolute error of the filtered mean: with the input 0.1880  without it 0.8884
```
### Example 5

Every refused argument.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }
static double max_abs(const pluss::Series &s) { return s.abs().max().item<double>(); }

// A double integrator x'' = u as the matrices of x' = A x + B u.
static pluss::Series A2() { return mat({0.0, 1.0, 0.0, 0.0}, 2, 2); }
static pluss::Series B2() { return mat({0.0, 1.0}, 2, 1); }

// A scalar random walk x[k+1] = x[k] + w, y[k] = x[k] + v, sampled every second.
static lti::StateSpace walk() { return lti::state(mat({1.0}, 1, 1), mat({0.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1), 1.0); }
// Eight observations of a constant 1 with a deterministic disturbance in place of noise.
static std::vector<double> readings(int n = 8) {
  std::vector<double> y;
  for (int k = 0; k < n; ++k) y.push_back(1.0 + 0.3 * std::sin(7.0 * k));
  return y;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series q = mat({0.1}, 1, 1), r = mat({1.0}, 1, 1), x0 = pluss::make_series({0.0}), p0 = mat({10.0}, 1, 1);
  refused(out, "a negative process covariance", [&] { ctl::kalman(walk(), mat(readings(), 8, 1), mat({-0.1}, 1, 1), r, x0, p0); });
  refused(out, "a negative noise covariance", [&] { ctl::kalman(walk(), mat(readings(), 8, 1), q, mat({-1.0}, 1, 1), x0, p0); });
  refused(out, "observations with two channels for a one-output system", [&] { ctl::kalman(walk(), mat({1.0, 1.0, 1.0, 1.0}, 2, 2), q, r, x0, p0); });
  refused(out, "an initial covariance of the wrong size", [&] { ctl::kalman(walk(), mat(readings(), 8, 1), q, r, x0, pluss::series::eye(2, pluss::ScalarType::Float64)); });
}
```

Output

```text
a negative process covariance : math::state: process_cov must be positive semidefinite
a negative noise covariance : math::state: observation_cov must be positive semidefinite
observations with two channels for a one-output system : math::state: observation must have shape [..., m, n]
an initial covariance of the wrong size : math::state: initial_cov must have shape [..., n, n]
```