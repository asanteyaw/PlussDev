<!-- generated from lqe.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::lqe`

Computes the steady-state Kalman gain of a state-space system.

## Signature

```cpp
control::LqeResult control::lqe(
    const lti::StateSpace& system,
    const pluss::Series& process_cov,
    const pluss::Series& noise_cov,
    pluss::ma::linearalg::MatrixEqOptions options = {});

control::LqeResult control::lqe(
    const lti::StateSpace& system,
    const pluss::Series& process_cov,
    const pluss::Series& noise_cov,
    const pluss::Series& cross,
    pluss::ma::linearalg::MatrixEqOptions options = {});
```

## What It Does

Solves the dual of the regulator Riccati equation for the steady-state estimation error covariance and returns it with the observer gain `L` and the estimator dynamics `A - L C`. A continuous system solves the continuous Riccati equation and a discrete one the discrete equation. An overload takes the cross-covariance between the process and measurement noise.

## Parameters

`system`
: A `StateSpace`; the pair `(A, C)` must be detectable.

`process_cov`, `noise_cov`
: `Q` and `R`; `R` must be positive definite.

`cross`
: Optional cross-covariance of the two noises (second overload).

`options`
: `MatrixEqOptions` of the underlying solver.

## Returns

An `LqeResult` with `covariance`, `gain` and `closed_loop`.

## Errors

A noise covariance that is not positive definite, covariances of the wrong size, and a system with an unstable mode that the outputs cannot see throw `pluss::Error`; the last names the detectability condition and appends the solver message.

## Interpretation and Use

For the discrete scalar walk the steady predicted covariance satisfies `p = p - p^2 / (p + r) + q`, which the first example verifies; the gain is `p / (p + r)` and the estimator pole `1 - gain`. For the continuous system `x' = -x + w` with unit covariances the covariance is `sqrt(2) - 1` (the second example). The third checks the Riccati residual for a two-state system.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A discrete scalar system against its closed form.

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
  // Discrete scalar walk: the steady predicted covariance p solves p = p - p^2 / (p + r) + q.
  const ctl::LqeResult l = ctl::lqe(walk(), mat({0.1}, 1, 1), mat({1.0}, 1, 1));
  const double p = l.covariance.flatten().items<double>()[0];
  out.writeln("steady covariance", p, " check p - p^2 / (p + 1) + 0.1 =", p - p * p / (p + 1.0) + 0.1);
  out.writeln("gain", l.gain.flatten().items<double>()[0], " expected p / (p + 1) =", p / (p + 1.0), " closed loop", l.closed_loop.flatten().items<double>()[0], " expected 1 - gain =", 1.0 - p / (p + 1.0));
}
```

Output

```text
steady covariance 0.3702  check p - p^2 / (p + 1) + 0.1 = 0.3702
gain 0.2702  expected p / (p + 1) = 0.2702  closed loop 0.7298  expected 1 - gain = 0.7298
```
### Example 2

A continuous scalar system against its closed form.

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
  // Continuous scalar system dx/dt = -x + w, y = x + v: 2 a p - p^2 / r + q = 0 gives p = r (a + sqrt(a^2 + q / r)).
  const lti::StateSpace s = lti::state(mat({-1.0}, 1, 1), mat({0.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1));
  const ctl::LqeResult l = ctl::lqe(s, mat({1.0}, 1, 1), mat({1.0}, 1, 1));
  const double p = -1.0 + std::sqrt(2.0);
  out.writeln("covariance", l.covariance.flatten().items<double>()[0], " expected", p, " gain", l.gain.flatten().items<double>()[0], " expected p / r =", p, " closed loop", l.closed_loop.flatten().items<double>()[0], " expected", -1.0 - p);
}
```

Output

```text
covariance 0.4142  expected 0.4142  gain 0.4142  expected p / r = 0.4142  closed loop -1.4142  expected -1.4142
```
### Example 3

The Riccati residual for a two-state system.

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
  const lti::StateSpace s = lti::state(mat({0.0, 1.0, 0.0, -0.5}, 2, 2), mat({0.0, 0.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1));
  const pluss::Series q = pluss::series::eye(2, pluss::ScalarType::Float64), r = mat({0.5}, 1, 1);
  const ctl::LqeResult l = ctl::lqe(s, q, r);
  const pluss::Series &p = l.covariance;
  const pluss::Series residual = s.a().matmul(p) + p.matmul(s.a().transpose(0, 1)) - p.matmul(s.c().transpose(0, 1)).matmul(s.c()).matmul(p).div(0.5) + q;
  out.writeln("covariance =", p.flatten().items<double>());
  out.writeln("largest entry of A P + P A' - P C' R^-1 C P + Q =", max_abs(residual));
}
```

Output

```text
covariance = [0.9132, 0.3340, 0.3340, 0.7769]
largest entry of A P + P A' - P C' R^-1 C P + Q = 0.0000
```
### Example 4

Every refused problem.

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
  const lti::StateSpace s = lti::state(mat({0.0, 1.0, 0.0, 0.0}, 2, 2), mat({0.0, 0.0}, 2, 1), mat({0.0, 1.0}, 1, 2), mat({0.0}, 1, 1));
  const pluss::Series q = pluss::series::eye(2, pluss::ScalarType::Float64);
  refused(out, "a noise covariance that is not positive definite", [&] { ctl::lqe(s, q, mat({-1.0}, 1, 1)); });
  refused(out, "a process covariance of the wrong size", [&] { ctl::lqe(s, mat({1.0}, 1, 1), mat({1.0}, 1, 1)); });
  const lti::StateSpace blind = lti::state(mat({1.0, 0.0, 0.0, 2.0}, 2, 2), mat({0.0, 0.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1));
  refused(out, "an unstable mode that the sensor cannot see", [&] { ctl::lqe(blind, q, mat({1.0}, 1, 1)); });
}
```

Output

```text
a noise covariance that is not positive definite : eng::control::lqe: noise_cov must be positive definite
a process covariance of the wrong size : eng::control::lqe: process_cov must have shape [..., states, states]
an unstable mode that the sensor cannot see : eng::control::lqe: no stabilising solution; (A, C) must be detectable (every unstable mode must be visible in the outputs). Solver message: subsp::math::linalg::lu_solve: singular matrix
```