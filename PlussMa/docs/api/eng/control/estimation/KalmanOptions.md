<!-- generated from KalmanOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::KalmanOptions`

The information threshold and covariance jitter of the Kalman filter.

## Signature

```cpp
control::KalmanOptions
```

## What It Does

Two numerical settings shared by `kalman` and `smooth`: an observation is used only when its innovation variance exceeds `info_tol`, and `jitter` is added to the predicted covariance at every step.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `info_tol` | Smallest innovation variance for which an observation is used; zero uses every finite observation. | `0` |
| `jitter` | Added to the diagonal of the predicted covariance at each step. | `1e-12` |

## Returns

Argument of `kalman` and `smooth`.

## Errors

A negative or non-finite `info_tol` or `jitter` throws `pluss::Error`.

## Interpretation and Use

Raise `info_tol` to ignore uninformative observations: with 2 the filter in the second example skips every sample whose innovation variance has fallen below 2 and uses it again once the covariance has grown back above it. A positive `jitter` keeps the covariance from collapsing and so keeps the gain from vanishing: 0.05 raises the final gain from 0.27 to 0.35 (the third example).

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The defaults.

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
  const ctl::KalmanOptions defaults;
  out.writeln("default info_tol is 0:", defaults.info_tol == 0.0, " default jitter is 1e-12:", defaults.jitter == 1e-12);
  const ctl::KalmanResult r = ctl::kalman(walk(), mat(readings(), 8, 1), mat({0.1}, 1, 1), mat({1.0}, 1, 1), pluss::make_series({0.0}), mat({10.0}, 1, 1));
  out.writeln("with the defaults every sample is used:", r.data_used.all().item<bool>());
}
```

Output

```text
default info_tol is 0: true  default jitter is 1e-12: true
with the defaults every sample is used: true
```
### Example 2

Which samples are used for three thresholds.

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
  // A sample is used only when its innovation variance exceeds info_tol.
  for (const double tol : {0.0, 2.0, 50.0}) {
    const ctl::KalmanResult r = ctl::kalman(walk(), mat(readings(), 8, 1), mat({0.1}, 1, 1), mat({1.0}, 1, 1), pluss::make_series({0.0}), mat({10.0}, 1, 1), {.info_tol = tol});
    out.writeln("info_tol", tol, ": data_used", r.data_used.flatten().items<bool>());
  }
}
```

Output

```text
info_tol 0.0000 : data_used [true, true, true, true, true, true, true, true]
info_tol 2.0000 : data_used [true, true, false, false, false, false, true, false]
info_tol 50.0000 : data_used [false, false, false, false, false, false, false, false]
```
### Example 3

The effect of the jitter on the final covariance and gain.

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
  // The jitter is added to the predicted covariance at every step, so a large value keeps the filter more responsive.
  for (const auto [label, jitter] : {std::pair<const char *, double>{"0", 0.0}, {"1e-12 (the default)", 1e-12}, {"0.05", 0.05}}) {
    const ctl::KalmanResult r = ctl::kalman(walk(), mat(readings(), 8, 1), mat({0.1}, 1, 1), mat({1.0}, 1, 1), pluss::make_series({0.0}), mat({10.0}, 1, 1), {.jitter = jitter});
    out.writeln("jitter", label, ": final filtered covariance", r.filtered_cov.flatten().items<double>().back(), " final gain", r.gain.flatten().items<double>().back());
  }
}
```

Output

```text
jitter 0 : final filtered covariance 0.2741  final gain 0.2741
jitter 1e-12 (the default) : final filtered covariance 0.2741  final gain 0.2741
jitter 0.05 : final filtered covariance 0.4093  final gain 0.3479
```
### Example 4

Every refused value.

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
  refused(out, "a negative info_tol", [&] { ctl::kalman(walk(), mat(readings(), 8, 1), mat({0.1}, 1, 1), mat({1.0}, 1, 1), pluss::make_series({0.0}), mat({10.0}, 1, 1), {.info_tol = -1.0}); });
  refused(out, "a negative jitter", [&] { ctl::kalman(walk(), mat(readings(), 8, 1), mat({0.1}, 1, 1), mat({1.0}, 1, 1), pluss::make_series({0.0}), mat({10.0}, 1, 1), {.jitter = -1.0}); });
}
```

Output

```text
a negative info_tol : math::state: info_tol must be finite and non-negative
a negative jitter : math::state: jitter must be finite and non-negative
```