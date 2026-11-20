<!-- generated from smooth.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::smooth`

Runs a Kalman filter and then a backward smoothing pass.

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

Computes the filter of `kalman` and then runs a smoothing pass backwards, so that every estimate uses all the observations, not only the earlier ones. It returns the embedded `KalmanResult` along with the smoothed mean and covariance and the smoother gains.

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
: Optional known input (second overload).

`options`
: `KalmanOptions`.

## Returns

A `SmoothResult`.

## Errors

The errors of `kalman`.

## Interpretation and Use

The smoothed covariance never exceeds the filtered covariance and the two agree at the last sample, where the smoother has no later data to use (the first and third examples). The accuracy gain is visible: against a true state of 1 the smoothed estimate has a root-mean-square error of 0.063 while the filtered one has 0.113 (the second example). Smooth when the whole record is available offline; filter when estimates are needed as the data arrive.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The smoothed covariance against the filtered one.

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
  const ctl::SmoothResult s = ctl::smooth(walk(), mat(readings(), 8, 1), mat({0.1}, 1, 1), mat({1.0}, 1, 1), pluss::make_series({0.0}), mat({10.0}, 1, 1));
  const auto filtered = s.filter.filtered_cov.flatten().items<double>(), smoothed = s.smoothed_cov.flatten().items<double>();
  out.writeln("filtered covariance =", filtered);
  out.writeln("smoothed covariance =", smoothed);
  bool never_larger = true;
  for (std::size_t k = 0; k < filtered.size(); ++k) never_larger = never_larger && smoothed[k] <= filtered[k] + 1e-12;
  out.writeln("the smoothed covariance never exceeds the filtered one:", never_larger, " and they agree at the last sample:", std::abs(filtered.back() - smoothed.back()) < 1e-12);
}
```

Output

```text
filtered covariance = [0.9099, 0.5025, 0.3760, 0.3225, 0.2970, 0.2842, 0.2776, 0.2741]
smoothed covariance = [0.2671, 0.2180, 0.1935, 0.1836, 0.1841, 0.1952, 0.2216, 0.2741]
the smoothed covariance never exceeds the filtered one: true  and they agree at the last sample: true
```
### Example 2

The error of the smoothed against the filtered estimate.

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
  // The truth is the constant 1; compare the error of the filtered and the smoothed estimates over all samples.
  const ctl::SmoothResult s = ctl::smooth(walk(), mat(readings(), 8, 1), mat({0.1}, 1, 1), mat({1.0}, 1, 1), pluss::make_series({0.0}), mat({10.0}, 1, 1));
  const auto f = s.filter.filtered_mean.flatten().items<double>(), m = s.smoothed_mean.flatten().items<double>();
  double ef = 0.0, em = 0.0;
  for (std::size_t k = 0; k < f.size(); ++k) { ef += (f[k] - 1.0) * (f[k] - 1.0); em += (m[k] - 1.0) * (m[k] - 1.0); }
  out.writeln("root-mean-square error of the filtered mean =", std::sqrt(ef / f.size()), " of the smoothed mean =", std::sqrt(em / m.size()));
}
```

Output

```text
root-mean-square error of the filtered mean = 0.1130  of the smoothed mean = 0.0625
```
### Example 3

The last sample and the smoother gains.

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
  const ctl::SmoothResult s = ctl::smooth(walk(), mat(readings(), 8, 1), mat({0.1}, 1, 1), mat({1.0}, 1, 1), pluss::make_series({0.0}), mat({10.0}, 1, 1));
  out.writeln("last smoothed mean", s.smoothed_mean.flatten().items<double>().back(), " last filtered mean", s.filter.filtered_mean.flatten().items<double>().back(), "(the smoother has no later data to use at the last sample)");
  out.writeln("smoother gains =", s.gain.flatten().items<double>(), "(one fewer than the samples)");
}
```

Output

```text
last smoothed mean 0.9035  last filtered mean 0.9035 (the smoother has no later data to use at the last sample)
smoother gains = [0.9010, 0.8340, 0.7899, 0.7633, 0.7481, 0.7397, 0.7351] (one fewer than the samples)
```
### Example 4

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
  const pluss::Series r = mat({1.0}, 1, 1), x0 = pluss::make_series({0.0}), p0 = mat({10.0}, 1, 1);
  refused(out, "a negative process covariance", [&] { ctl::smooth(walk(), mat(readings(), 8, 1), mat({-0.1}, 1, 1), r, x0, p0); });
  refused(out, "observations with two channels for a one-output system", [&] { ctl::smooth(walk(), mat({1.0, 1.0, 1.0, 1.0}, 2, 2), mat({0.1}, 1, 1), r, x0, p0); });
}
```

Output

```text
a negative process covariance : math::state: process_cov must be positive semidefinite
observations with two channels for a one-output system : math::state: observation must have shape [..., m, n]
```