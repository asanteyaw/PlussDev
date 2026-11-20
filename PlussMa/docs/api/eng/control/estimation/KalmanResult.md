<!-- generated from KalmanResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::KalmanResult`

The sequence of estimates produced by `kalman`.

## Signature

```cpp
control::KalmanResult
```

## What It Does

One entry per sample for each field: the filtered and predicted state, the observation prediction and innovation, the gain and the likelihood terms.

## Fields

| Field | Meaning |
|---|---|
| `filtered_mean`, `filtered_cov` | The state estimate and its covariance after using the observation. |
| `predicted_mean`, `predicted_cov` | The same before using the observation. |
| `predicted_obs` | The predicted observation. |
| `innovation`, `innovation_cov` | The observation minus its prediction, and the variance of that. |
| `gain` | The Kalman gain at each sample. |
| `data_used` | Whether each observation channel was used. |
| `loglik_terms`, `loglik` | The log-likelihood contribution of each sample and their sum. |

## Returns

Returned by `kalman` and embedded in `SmoothResult`.

## Errors

Reading a field does not run again.

## Interpretation and Use

The innovation equals the observation minus `predicted_obs`, and its variance is `C P C' + R` (the third example checks it equals `predicted_cov + R` here). The log-likelihood is the sum of the per-sample terms (second example) and is what to maximise when estimating `Q` and `R` from data.

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
  const ctl::KalmanResult r = ctl::kalman(walk(), mat(readings(), 8, 1), mat({0.1}, 1, 1), mat({1.0}, 1, 1), pluss::make_series({0.0}), mat({10.0}, 1, 1));
  out.writeln("filtered_mean", r.filtered_mean.size(0), "x", r.filtered_mean.size(1), " filtered_cov", r.filtered_cov.size(0), "x", r.filtered_cov.size(1), "x", r.filtered_cov.size(2));
  out.writeln("predicted_mean", r.predicted_mean.size(0), "x", r.predicted_mean.size(1), " predicted_obs", r.predicted_obs.size(0), "x", r.predicted_obs.size(1), " innovation", r.innovation.size(0), "x", r.innovation.size(1));
  out.writeln("gain", r.gain.size(0), "x", r.gain.size(1), "x", r.gain.size(2), " data_used", r.data_used.size(0), "x", r.data_used.size(1), " loglik_terms", r.loglik_terms.numel(), " loglik", r.loglik.numel(), "value");
}
```

Output

```text
filtered_mean 8 x 1  filtered_cov 8 x 1 x 1
predicted_mean 8 x 1  predicted_obs 8 x 1  innovation 8 x 1
gain 8 x 1 x 1  data_used 8 x 1  loglik_terms 8  loglik 1 value
```
### Example 2

The innovation and the likelihood from their parts.

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
  const auto y = readings();
  const ctl::KalmanResult r = ctl::kalman(walk(), mat(readings(), 8, 1), mat({0.1}, 1, 1), mat({1.0}, 1, 1), pluss::make_series({0.0}), mat({10.0}, 1, 1));
  const auto innovation = r.innovation.flatten().items<double>(), predicted = r.predicted_obs.flatten().items<double>();
  double worst = 0.0;
  for (std::size_t k = 0; k < y.size(); ++k) worst = std::max(worst, std::abs(innovation[k] - (y[k] - predicted[k])));
  out.writeln("innovation = observation - predicted observation, largest difference", worst);
  const auto terms = r.loglik_terms.flatten().items<double>();
  double sum = 0.0;
  for (const double t : terms) sum += t;
  out.writeln("loglik =", r.loglik.item<double>(), " sum of loglik_terms =", sum);
}
```

Output

```text
innovation = observation - predicted observation, largest difference 0.0000
loglik = -10.1875  sum of loglik_terms = -10.1875
```
### Example 3

The innovation covariance.

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
  // The innovation covariance is the predicted variance of the observation: predicted_cov + R for this scalar system.
  const ctl::KalmanResult r = ctl::kalman(walk(), mat(readings(), 8, 1), mat({0.1}, 1, 1), mat({1.0}, 1, 1), pluss::make_series({0.0}), mat({10.0}, 1, 1));
  const auto s = r.innovation_cov.flatten().items<double>(), p = r.predicted_cov.flatten().items<double>();
  double worst = 0.0;
  for (std::size_t k = 0; k < s.size(); ++k) worst = std::max(worst, std::abs(s[k] - (p[k] + 1.0)));
  out.writeln("innovation_cov =", s);
  out.writeln("largest difference from predicted_cov + R =", worst);
}
```

Output

```text
innovation_cov = [11.1000, 2.0099, 1.6025, 1.4760, 1.4225, 1.3970, 1.3842, 1.3776]
largest difference from predicted_cov + R = 0.0000
```