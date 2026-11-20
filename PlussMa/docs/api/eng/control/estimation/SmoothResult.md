<!-- generated from SmoothResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::SmoothResult`

The filtered and smoothed estimates produced by `smooth`.

## Signature

```cpp
control::SmoothResult
```

## What It Does

Holds the full `KalmanResult` from the forward pass together with the smoothed mean and covariance and the backward gains.

## Fields

| Field | Meaning | Shape |
|---|---|---|
| `filter` | The `KalmanResult` of the forward pass. | |
| `smoothed_mean` | The state estimate using all observations. | `[time, states]` |
| `smoothed_cov` | Its covariance. | `[time, states, states]` |
| `gain` | The backward smoother gains. | `[time - 1, states, states]` |

## Returns

Returned by `smooth`.

## Errors

Reading a field does not run again.

## Interpretation and Use

`filter` is exactly what `kalman` returns for the same arguments (the second example finds no difference in the mean or the likelihood). There is one fewer smoother gain than samples, because the last sample has nothing later to smooth with.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The shape of every field.

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
  out.writeln("filter.filtered_mean", s.filter.filtered_mean.size(0), "x", s.filter.filtered_mean.size(1), " smoothed_mean", s.smoothed_mean.size(0), "x", s.smoothed_mean.size(1), " smoothed_cov", s.smoothed_cov.size(0), "x", s.smoothed_cov.size(1), "x", s.smoothed_cov.size(2), " gain", s.gain.size(0), "x", s.gain.size(1), "x", s.gain.size(2));
}
```

Output

```text
filter.filtered_mean 8 x 1  smoothed_mean 8 x 1  smoothed_cov 8 x 1 x 1  gain 7 x 1 x 1
```
### Example 2

The embedded filter against a direct `kalman` call.

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
  const ctl::KalmanResult f = ctl::kalman(walk(), mat(readings(), 8, 1), mat({0.1}, 1, 1), mat({1.0}, 1, 1), pluss::make_series({0.0}), mat({10.0}, 1, 1));
  out.writeln("the embedded filter equals a direct kalman call: largest difference in filtered_mean", max_abs(s.filter.filtered_mean - f.filtered_mean), " in loglik", std::abs(s.filter.loglik.item<double>() - f.loglik.item<double>()));
}
```

Output

```text
the embedded filter equals a direct kalman call: largest difference in filtered_mean 0.0000  in loglik 0.0000
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