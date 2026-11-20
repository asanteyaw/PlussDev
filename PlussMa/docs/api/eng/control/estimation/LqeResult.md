<!-- generated from LqeResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::LqeResult`

The covariance, gain and estimator dynamics of `lqe`.

## Signature

```cpp
control::LqeResult
```

## What It Does

Returned by `lqe`. It holds the steady-state estimation covariance, the observer gain and the matrix `A - L C` that governs the estimation error.

## Fields

| Field | Meaning | Shape |
|---|---|---|
| `covariance` | The steady-state error covariance `P`. | `[..., n, n]` |
| `gain` | The observer gain `L`. | `[..., n, p]` |
| `closed_loop` | `A - L C` (discrete: the equivalent error dynamics). | `[..., n, n]` |

## Returns

Returned by `lqe`.

## Errors

Reading a field does not run again.

## Interpretation and Use

The steady gain is the limit of the time-varying Kalman gain: after 40 samples the filter gain equals the `lqe` gain to four places (the second example). `closed_loop` has all its eigenvalues stable, which is the reason to use the steady gain in an observer.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The three fields for the scalar walk.

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
  const ctl::LqeResult l = ctl::lqe(walk(), mat({0.1}, 1, 1), mat({1.0}, 1, 1));
  out.writeln("covariance", l.covariance.flatten().items<double>(), " gain", l.gain.flatten().items<double>(), " closed loop", l.closed_loop.flatten().items<double>());
}
```

Output

```text
covariance [0.3702]  gain [0.2702]  closed loop [0.7298]
```
### Example 2

The `lqe` gain against the gain of a long Kalman run.

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
  // The steady Kalman gain from lqe is where the time-varying gain of kalman ends up.
  const ctl::KalmanResult long_run = ctl::kalman(walk(), mat(readings(40), 40, 1), mat({0.1}, 1, 1), mat({1.0}, 1, 1), pluss::make_series({0.0}), mat({10.0}, 1, 1));
  const ctl::LqeResult l = ctl::lqe(walk(), mat({0.1}, 1, 1), mat({1.0}, 1, 1));
  out.writeln("gain after 40 samples =", long_run.gain.flatten().items<double>().back(), " steady-state gain from lqe =", l.gain.flatten().items<double>()[0]);
}
```

Output

```text
gain after 40 samples = 0.2702  steady-state gain from lqe = 0.2702
```